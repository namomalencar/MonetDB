/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2016 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * Martin Kersten
 * Petri-net continuous query scheduler
   The CQ scheduler is based on the long-standing and mature Petri-net technology. For completeness, we
   recap its salient points taken from Wikipedia. For more detailed information look at the science library.

   The cquery scheduler is a fair implementation of a Petri-net interpreter. It models all continuous queries as transitions,
   and the stream tables represent the places with all tokens. The firing condition is determined by an external clock
   or availability of sufficient number of tokens (set by window()). Unlike the pure Petri-net model, the number of tokens 
   taken out on each firing is set by the tumble().  
   The firing rule is an ordinary SQL procedure. It may result into placing multiple tokens into receiving baskets.

   The scheduling amongst the transistions is currently deterministic. Upon each round of the scheduler, it determines all
   transitions eligble to fire, i.e. have non-empty baskets or whose heartbeat ticks, which are then actived one after the other.
   Future implementations may relax this rigid scheme using a parallel implementation of the scheduler, such that each 
   transition by itself can decide to fire. However, when resources are limited to handle all complex continuous queries, 
   it may pay of to invest into a domain specif scheduler.

   The current implementation is limited to a fixed number of transitions. The scheduler can be stopped and restarted
   at any time. Even selectively for specific baskets. This provides the handle to debug a system before being deployed.
   In general, event processing through multiple layers of continous queries is too fast to trace them one by one.
   Some general statistics about number of events handled per transition is maintained, as well as the processing time
   for each continous query step. This provides the information to re-design the event handling system.
 */

#include "monetdb_config.h"
#include "sql_optimizer.h"
#include "sql_gencode.h"
#include "sql_cquery.h"
#include "sql_basket.h"
#include "mal_builder.h"
#include "opt_prelude.h"

static str statusname[7] = { "init", "register", "readytorun", "running", "waiting", "paused","stopping"};

static str CQstartScheduler(void);
static int CQinit;
static int pnstatus;

static BAT *CQ_id_tick = 0;
static BAT *CQ_id_mod = 0;
static BAT *CQ_id_fcn = 0;
static BAT *CQ_id_time = 0;
static BAT *CQ_id_error = 0;

CQnode pnet[MAXCQ];
int pnettop = 0;

static int pnstatus = CQINIT;
static int cycleDelay = 200; /* be careful, it affects response/throughput timings */
MT_Lock ttrLock MT_LOCK_INITIALIZER("cqueryLock");

static void
CQfree(int idx)
{
	int j;
	MT_lock_set(&ttrLock);
	if( pnet[idx].mb)
		freeMalBlk(pnet[idx].mb);
	if( pnet[idx].stk)
		GDKfree(pnet[idx].stk);
	for( j=0; j< MAXSTREAMS && pnet[idx].schema[j]; j++){
		GDKfree(pnet[idx].schema[j]);
		GDKfree(pnet[idx].tables[j]);
		GDKfree(pnet[idx].column[j]);
		if( pnet[idx].bats[j])
			BBPunfix(pnet[idx].bats[j]->batCacheid);
	}
	GDKfree(pnet[idx].mod);
	GDKfree(pnet[idx].fcn);
	for( ; idx<pnettop-1; idx++)
		pnet[idx] = pnet[idx+1];
	pnettop--;
	memset((void*) (pnet+idx), 0, sizeof(CQnode));
	MT_lock_unset(&ttrLock);
}

/* We need a lock table for all stream tables considered
 * It is better to use a slot in the BATdescriptor 
 * A sanity routine should be available to check for any forgotten lock frees.
 */

/*
static str
CQallLocksReleased(void)
{
	return MAL_SUCCEED;
}
*/

/* Initialization phase of scheduler */
static str
CQcleanuplog(void)
{
#ifdef DEBUG_CQUERY
	fprintf(stderr,"#cleaup query.log table\n");
#endif
	return MAL_SUCCEED;
}

static str
CQcreatelog(void){
	if( CQ_id_tick)
		return MAL_SUCCEED;
#ifdef DEBUG_CQUERY
	fprintf(stderr,"#create query.log table\n");
#endif
	CQ_id_tick = COLnew(0, TYPE_timestamp, 1<<16, TRANSIENT);
	CQ_id_mod = COLnew(0, TYPE_str, 1<<16, TRANSIENT);
	CQ_id_fcn = COLnew(0, TYPE_str, 1<<16, TRANSIENT);
	CQ_id_time = COLnew(0, TYPE_lng, 1<<16, TRANSIENT);
	CQ_id_error = COLnew(0, TYPE_str, 1<<16, TRANSIENT);
	if ( CQ_id_tick == 0 &&
		CQ_id_mod == 0 &&
		CQ_id_fcn == 0 &&
		CQ_id_time == 0 &&
		CQ_id_error == 0){
			(void) CQcleanuplog();
			throw(MAL,"cquery.log",MAL_MALLOC_FAIL);
		}
	return MAL_SUCCEED;
}

static void
CQentry(int idx)
{
	CQcreatelog();
	if( BUNappend(CQ_id_tick, &pnet[idx].seen,FALSE) != GDK_SUCCEED ||
		BUNappend(CQ_id_mod, pnet[idx].mod,FALSE) != GDK_SUCCEED ||
		BUNappend(CQ_id_fcn, pnet[idx].fcn,FALSE) != GDK_SUCCEED ||
		BUNappend(CQ_id_time, &pnet[idx].time,FALSE) != GDK_SUCCEED ||
		BUNappend(CQ_id_error, (pnet[idx].error ? pnet[idx].error:""),FALSE) != GDK_SUCCEED )
		pnet[idx].error = createException(SQL,"cquery.logentry",MAL_MALLOC_FAIL);
}

str
CQlog( Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci){
	BAT *tickbat, *modbat, *fcnbat, *timebat, *errbat;
	bat *tickret, *modret, *fcnret, *timeret, *errorret;
	
	(void) cntxt;
	(void) mb;

	tickret = getArgReference_bat(stk, pci, 0);
	modret = getArgReference_bat(stk, pci, 1);
	fcnret = getArgReference_bat(stk, pci, 2);
	timeret = getArgReference_bat(stk, pci, 3);
	errorret = getArgReference_bat(stk, pci, 4);
#ifdef DEBUG_CQUERY
	fprintf(stderr,"#produce query.log table\n");
#endif
	CQcreatelog();
	tickbat = COLcopy(CQ_id_tick, TYPE_timestamp, 0, TRANSIENT);
	if(tickbat == NULL)
		goto wrapup;
	modbat = COLcopy(CQ_id_mod, TYPE_str, 0, TRANSIENT);
	if(modbat == NULL)
		goto wrapup;
	fcnbat = COLcopy(CQ_id_fcn, TYPE_str, 0, TRANSIENT);
	if(fcnbat == NULL)
		goto wrapup;
	timebat = COLcopy(CQ_id_time, TYPE_lng, 0, TRANSIENT);
	if(timebat == NULL)
		goto wrapup;
	errbat = COLcopy(CQ_id_error, TYPE_str, 0, TRANSIENT);
	if(errbat == NULL)
		goto wrapup;
	BBPkeepref(*tickret = tickbat->batCacheid);
	BBPkeepref(*modret = modbat->batCacheid);
	BBPkeepref(*fcnret = fcnbat->batCacheid);
	BBPkeepref(*timeret = timebat->batCacheid);
	BBPkeepref(*errorret = errbat->batCacheid);
	return MAL_SUCCEED;
wrapup:
	if( tickbat) BBPunfix(tickbat->batCacheid);
	if( modbat) BBPunfix(modbat->batCacheid);
	if( fcnbat) BBPunfix(fcnbat->batCacheid);
	if( timebat) BBPunfix(timebat->batCacheid);
	if( errbat) BBPunfix(errbat->batCacheid);
	return MAL_SUCCEED;
}

str
CQstatus( Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci){
	BAT *tickbat, *modbat, *fcnbat, *statusbat, *errbat;
	bat *tickret, *modret, *fcnret, *statusret, *errorret;
	int idx;
	str msg= MAL_SUCCEED;
	
	(void) cntxt;
	(void) mb;

	tickret = getArgReference_bat(stk, pci, 0);
	modret = getArgReference_bat(stk, pci, 1);
	fcnret = getArgReference_bat(stk, pci, 2);
	statusret = getArgReference_bat(stk, pci, 3);
	errorret = getArgReference_bat(stk, pci, 4);

	tickbat = COLnew(0, TYPE_timestamp, 0, TRANSIENT);
	if(tickbat == NULL)
		goto wrapup;
	modbat = COLnew(0, TYPE_str, 0, TRANSIENT);
	if(modbat == NULL)
		goto wrapup;
	fcnbat = COLnew(0, TYPE_str, 0, TRANSIENT);
	if(fcnbat == NULL)
		goto wrapup;
	statusbat = COLnew(0, TYPE_str, 0, TRANSIENT);
	if(statusbat == NULL)
		goto wrapup;
	errbat = COLnew(0, TYPE_str, 0, TRANSIENT);
	if(errbat == NULL)
		goto wrapup;

	for( idx = 0; msg == MAL_SUCCEED && idx < pnettop; idx++)
		if( BUNappend(tickbat, &pnet[idx].seen,FALSE) != GDK_SUCCEED ||
			BUNappend(modbat, pnet[idx].mod,FALSE) != GDK_SUCCEED ||
			BUNappend(fcnbat, pnet[idx].fcn,FALSE) != GDK_SUCCEED ||
			BUNappend(statusbat, statusname[pnet[idx].status],FALSE) != GDK_SUCCEED ||
			BUNappend(errbat, (pnet[idx].error ? pnet[idx].error:""),FALSE) != GDK_SUCCEED )
				msg = createException(SQL,"cquery.status",MAL_MALLOC_FAIL);

	BBPkeepref(*tickret = tickbat->batCacheid);
	BBPkeepref(*modret = modbat->batCacheid);
	BBPkeepref(*fcnret = fcnbat->batCacheid);
	BBPkeepref(*statusret = statusbat->batCacheid);
	BBPkeepref(*errorret = errbat->batCacheid);
	return msg;
wrapup:
	if( tickbat) BBPunfix(tickbat->batCacheid);
	if( modbat) BBPunfix(modbat->batCacheid);
	if( fcnbat) BBPunfix(fcnbat->batCacheid);
	if( statusbat) BBPunfix(statusbat->batCacheid);
	if( errbat) BBPunfix(errbat->batCacheid);
	throw(SQL,"cquery.status",MAL_MALLOC_FAIL);
}

static int
CQlocate(str modname, str fcnname)
{
	int i;

	for (i = 0; i < pnettop; i++){
		if (strcmp(pnet[i].mod, modname) == 0 && strcmp(pnet[i].fcn, fcnname) == 0)
			return i;
	}
	return i;
}

/* capture and remember errors */
str
CQerror(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch = *getArgReference_str(stk,pci,1);
	str fcn = *getArgReference_str(stk,pci,2);
	str error = *getArgReference_str(stk,pci,3);
	int idx;

	(void) cntxt;
	(void) mb;

	idx = CQlocate(sch, fcn);
	if( idx == pnettop)
		throw(SQL,"cquery.error","Continous query %s.%s not accessible\n",sch,fcn);

	pnet[idx].error = GDKstrdup(error);
	return MAL_SUCCEED;
}

/* A debugging routine */
str
CQshow(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch = *getArgReference_str(stk,pci,1);
	str fcn = *getArgReference_str(stk,pci,2);
	int idx;

	(void) cntxt;
	(void) mb;

	idx = CQlocate(sch, fcn);
	if( idx == pnettop)
		throw(SQL,"cquery.show","Continous query %s.%s not accessible\n",sch,fcn);

	printFunction(cntxt->fdout, pnet[idx].mb, 0, LIST_MAL_NAME | LIST_MAL_VALUE  | LIST_MAL_MAPI);
	return MAL_SUCCEED;
}

/* Collect all input/output basket roles */
/* Make sure we do not re-use the same source more than once */
/* Avoid any concurreny conflict */
static str
CQanalysis(Client cntxt, MalBlkPtr mb, int pn)
{
	int i, j, idx, bskt;
	InstrPtr p;
	str msg= MAL_SUCCEED, sch, tbl;
	(void) cntxt;

	p = getInstrPtr(mb, 0);
	idx = CQlocate(getModuleId(p), getFunctionId(p));
	if( idx != pnettop)
		throw(MAL,"cquery.analysis","Duplicate or unknown registration of %s.%s \n", getModuleId(p), getFunctionId(p));
	MT_lock_unset(&ttrLock);
	for (i = 0; msg== MAL_SUCCEED && i < mb->stop; i++) {
		p = getInstrPtr(mb, i);
		if (getModuleId(p) == basketRef && getFunctionId(p) == registerRef){
			sch = getVarConstant(mb, getArg(p,2)).val.sval;
			tbl = getVarConstant(mb, getArg(p,3)).val.sval;

			// find the stream basket definition 
			bskt = BSKTlocate(sch,tbl);
			if( bskt == 0){
				msg = BSKTregisterInternal(cntxt,mb,sch,tbl);
				if( msg == MAL_SUCCEED){
					bskt = BSKTlocate(sch,tbl);
					if( bskt == 0){
						msg = createException(MAL,"cquery.analysis","basket registration missing\n");
						continue;
					}
				}
			}
			
			// we only need a single column for window size testing
			for( j=0; j< MAXSTREAMS && pnet[pn].schema[j]; j++)
			if( strcmp(sch, pnet[pn].schema[j]) == 0 &&
				strcmp(tbl, pnet[pn].tables[j]) == 0 )
				break;
			if ( j == MAXSTREAMS){
				msg = createException(MAL,"cquery.analysis","too many stream table columns\n");
				continue;
			}

			if ( pnet[pn].schema[j] )
				continue;

			pnet[pn].schema[j] = GDKstrdup(sch);
			pnet[pn].tables[j] = GDKstrdup(tbl);
			pnet[pn].column[j] = GDKstrdup(baskets[bskt].cols[0]);
			pnet[pn].window[j] = baskets[bskt].window;
			pnet[pn].stride[j] = baskets[bskt].stride;
		}
	}
	MT_lock_set(&ttrLock);
	if( msg != MAL_SUCCEED){
		// restore the state for later re-use
		CQfree(pn);
	}
	return msg;
}

// locate the SQL procedure in the catalog
static str
IOTprocedureStmt(Client cntxt, MalBlkPtr mb, str schema, str nme)
{
    mvc *m = NULL;
    str msg = MAL_SUCCEED;
    sql_schema  *s;
    backend *be;
    node *o;
    sql_func *f;
    /*sql_trans *tr;*/

    msg = getSQLContext(cntxt, mb, &m, NULL);
    if ((msg = checkSQLContext(cntxt)) != MAL_SUCCEED)
        return msg;
    s = mvc_bind_schema(m, schema);
    if (s == NULL)
        throw(SQL, "cquery.register", "Schema missing");
    /*tr = m->session->tr;*/
    for (o = s->funcs.set->h; o; o = o->next) {
        f = o->data;
        if (strcmp(f->base.name, nme) == 0) {
            be = (void *) backend_create(m, cntxt);
            if ( be->mvc->sa == NULL)
                be->mvc->sa = sa_create();
            //TODO fix result type
            backend_create_func(be, f, f->res,NULL);
            return MAL_SUCCEED;
        }
    }
    throw(SQL, "cquery.register", "SQL procedure missing");
}

str
CQregister(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	int i;
	InstrPtr sig,q;
	str msg = MAL_SUCCEED;
	MalBlkPtr nmb;
	Symbol s;
	Module scope;
	char buf[IDLENGTH];

	str modnme = *getArgReference_str(stk, pci, 1);
	str fcnnme = *getArgReference_str(stk, pci, 2);

    msg = IOTprocedureStmt(cntxt, mb, modnme, fcnnme);
	if( msg)
		return msg;

	scope = findModule(cntxt->nspace, putName(modnme));
	if (scope)
		s = findSymbolInModule(scope, putName(fcnnme));

	if (s == NULL)
		throw(MAL, "cquery.register", "Could not find SQL procedure\n");

	if (pnettop == MAXCQ) 
		GDKerror("cquery.register:Too many transitions");

	mb = s->def;
	sig = getInstrPtr(mb,0);
	i = CQlocate(getModuleId(sig), getFunctionId(sig));
	if (i != pnettop)
		throw(MAL,"cquery.register","Duplicate registration of cquery");

#ifdef DEBUG_CQUERY
	fprintf(stderr, "#cquery register %s.%s\n", getModuleId(sig),getFunctionId(sig));
	fprintFunction(stderr,mb,0,LIST_MAL_ALL);
#endif
	memset((void*) (pnet+pnettop), 0, sizeof(CQnode));

	snprintf(buf,IDLENGTH,"%s_%s",modnme,fcnnme);
	s = newFunction(userRef, putName(buf), FUNCTIONsymbol);
	nmb = s->def;
	setArgType(nmb, nmb->stmt[0],0, TYPE_void);
    (void) newStmt(nmb, sqlRef, transactionRef);
	(void) newStmt(nmb, getModuleId(sig),getFunctionId(sig));
    q = newStmt(nmb, sqlRef, commitRef);
	setArgType(nmb,q, 0, TYPE_void);
	pushEndInstruction(nmb);
	chkProgram(cntxt->fdout, cntxt->nspace, nmb);
#ifdef DEBUG_CQUERY
	fprintFunction(stderr, nmb, 0, LIST_MAL_ALL);
#endif

	MT_lock_set(&ttrLock);
	if( CQlocate(getModuleId(sig), getFunctionId(sig)) != pnettop){
		freeSymbol(s);
		throw(MAL,"cquery.register","Duplicate registration of cquery");
	}
	pnet[pnettop].mod = GDKstrdup(modnme);
	pnet[pnettop].fcn = GDKstrdup(fcnnme);
	pnet[pnettop].mb = nmb;
	pnet[pnettop].stk = prepareMALstack(nmb, nmb->vsize);

	pnet[pnettop].cycles = int_nil; 
	pnet[pnettop].beats = lng_nil;
	pnet[pnettop].run  = lng_nil;
	pnet[pnettop].seen = *timestamp_nil;
	pnet[pnettop].status = CQPAUSE;
	pnettop++;

	msg = CQanalysis(cntxt, mb, pnettop-1);
	MT_lock_unset(&ttrLock);
	if( msg != MAL_SUCCEED)
		// restore the entry
		CQfree(pnettop);
	if( pnettop == 0)
		pnstatus = CQSTOP;
	return msg;
}

str
CQresume(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch, fcn;
	int idx=0, last= pnettop;
	str msg = MAL_SUCCEED;
	(void) cntxt;
	(void) mb;

	if( pci->argc >2){
		sch = *getArgReference_str(stk,pci,1);
		fcn = *getArgReference_str(stk,pci,2);
		idx = CQlocate(sch, fcn);
		if( idx == pnettop)
			throw(SQL,"cquery.resume","continuous query %s.%s not accessible\n",sch,fcn);
		last = idx+1;
	}
#ifdef DEBUG_CQUERY
	fprintf(stderr, "#resume scheduler\n");
#endif
	MT_lock_set(&ttrLock);
	for( ; idx < last; idx++)
		pnet[idx].status = CQWAIT;
	MT_lock_unset(&ttrLock);

	/* start the scheduler if needed */
	if(CQinit == 0) {
		msg = CQstartScheduler();
	}
	return msg;
}

str
CQpause(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch, fcn;
	int idx=0, last= pnettop;
	(void) cntxt;
	(void) mb;

	if( pci->argc >2){
		sch = *getArgReference_str(stk,pci,1);
		fcn = *getArgReference_str(stk,pci,2);
		idx = CQlocate(sch, fcn);
		if( idx == pnettop)
			throw(SQL,"cquery.pause","continuous query %s.%s not accessible\n",sch,fcn);
		last = idx+1;
	}
#ifdef DEBUG_CQUERY
	fprintf(stderr, "#pause cqueries\n");
#endif
	MT_lock_set(&ttrLock);
	for( ; idx < last; idx++)
		pnet[idx].status = CQPAUSE;
	MT_lock_unset(&ttrLock);
	return MAL_SUCCEED;
}

str
CQcycles(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch, fcn;
	int cycles, idx=0, last= pnettop;
	(void) cntxt;
	(void) mb;

	if( pci->argc >2){
		sch = *getArgReference_str(stk,pci,1);
		fcn = *getArgReference_str(stk,pci,2);
		idx = CQlocate(sch, fcn);
		if( idx == pnettop)
			throw(SQL,"cquery.pause","continuous query %s.%s not accessible\n",sch,fcn);
		last = idx+1;
		cycles = *getArgReference_int(stk,pci,3);
	} else
		cycles = *getArgReference_int(stk,pci,1);
#ifdef DEBUG_CQUERY
	fprintf(stderr, "#set cycles \n");
#endif
	MT_lock_set(&ttrLock);
	for( ; idx < last; idx++)
		pnet[idx].cycles = cycles;
	MT_lock_unset(&ttrLock);
	return MAL_SUCCEED;
}

static void
CQsetbeat(int idx, int beats)
{
	int i;
	for( i=0; i < MAXSTREAMS; i++)
	if( pnet[idx].inout[i] == STREAM_IN && pnet[idx].window[i]){
		// can not set the beat due to stream window constraint
		pnet[idx].beats = lng_nil;
		return;
	}
	pnet[idx].beats = beats;
}

str
CQheartbeat(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch, fcn;
	int beats, idx=0, last= pnettop;
	str msg = MAL_SUCCEED;
	(void) cntxt;
	(void) mb;

	if( pci->argc >2){
		sch = *getArgReference_str(stk,pci,1);
		fcn = *getArgReference_str(stk,pci,2);
		idx = CQlocate(sch, fcn);
		if( idx == pnettop)
			throw(SQL,"cquery.pause","continuous query %s.%s not accessible\n",sch,fcn);
		last = idx+1;
		beats = *getArgReference_int(stk,pci,3);
#ifdef DEBUG_CQUERY
		fprintf(stderr, "#set the heartbeat of %s.%s to %d\n",sch,fcn,beats);
#endif
	} else{
		beats = *getArgReference_int(stk,pci,1);
#ifdef DEBUG_CQUERY
		fprintf(stderr, "#set the heartbeat %d ms\n",beats);
#endif
	}
	MT_lock_set(&ttrLock);
	for( ; idx < last; idx++)
		CQsetbeat(idx, beats *1000); /* minimal 1 ms */
	MT_lock_unset(&ttrLock);
	/* start the scheduler if needed */
	if(CQinit == 0) {
		msg = CQstartScheduler();
	}
	return msg;
}

str
CQwait(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci){
	unsigned int delay = (unsigned int) *getArgReference_int(stk,pci,1);

	(void) cntxt;
	(void) mb;
#ifdef DEBUG_CQUERY
	fprintf(stderr, "#scheduler wait %d ms\n",delay);
#endif
	MT_sleep_ms(delay);
	return MAL_SUCCEED;
}

/*Remove a specific continuous query from the scheduler */

str
CQderegister(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci){
	str sch, fcn;
	int idx=0, last= pnettop;

	(void) cntxt;
	(void) mb;

	if( pci->argc >2){
		sch = *getArgReference_str(stk,pci,1);
		fcn = *getArgReference_str(stk,pci,2);
		idx = CQlocate(sch, fcn);
		if( idx == pnettop)
			throw(SQL,"cquery.deregister","continuous query %s.%s not accessible\n",sch,fcn);
		last = idx+1;
	}
#ifdef DEBUG_CQUERY
	fprintf(stderr, "#deregister queries %d - %d\n",idx,last);
#endif
	for( ; idx < last; idx++)
		CQfree(idx);
	return MAL_SUCCEED;
}

str
CQtumble(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch= *getArgReference_str(stk,pci,1);
	str tbl= *getArgReference_str(stk,pci,2);
	int elm= *getArgReference_int(stk,pci,3);
	int i,j;

#ifdef DEBUG_CQUERY
	fprintf(stderr, "#cquery.tumble %s.%s %d\n",sch,tbl,elm);
#endif
	for(j=0;j<pnettop; j++)
	for(i = 0; i< MAXSTREAMS; i++)
	if( pnet[j].schema[i] && strcmp(sch, pnet[j].schema[i]) == 0 && 
		strcmp(tbl,pnet[j].tables[i])== 0 &&
		pnet[j].inout[i] == STREAM_IN)
		pnet[j].stride[i] = elm;
	(void) cntxt;
	(void) mb;
	return MAL_SUCCEED;
}

str
CQwindow(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	str sch= *getArgReference_str(stk,pci,1);
	str tbl= *getArgReference_str(stk,pci,2);
	int elm= *getArgReference_int(stk,pci,3);
	int i,j;

#ifdef DEBUG_CQUERY
	fprintf(stderr, "#cquery.window %s.%s %d\n",sch,tbl,elm);
#endif
	for(j=0;j<pnettop; j++)
	for(i = 0; i< MAXSTREAMS; i++)
	if( pnet[j].schema[i] && strcmp(sch, pnet[j].schema[i]) == 0 && 
		strcmp(tbl,pnet[j].tables[i])== 0 &&
		pnet[j].inout[i] == STREAM_IN)
			pnet[j].window[i] = elm;
	(void) cntxt;
	(void) mb;
	return MAL_SUCCEED;
}

str
CQdump(void *ret)
{
	int i, k;

	fprintf(stderr, "#scheduler status %s\n", statusname[pnstatus]);
	for (i = 0; i < pnettop; i++) {
		fprintf(stderr, "#[%d]\t%s.%s %s ",
				i, pnet[i].mod, pnet[i].fcn, statusname[pnet[i].status]);
		if ( pnet[i].beats != lng_nil)
			fprintf(stderr, "beats="LLFMT" ", pnet[i].beats);
			
		if( pnet[i].inout[0])
			fprintf(stderr, " streams ");
		for (k = 0; k < MAXSTREAMS && pnet[i].schema[k]; k++)
		if( pnet[i].inout[k] == STREAM_IN)
			fprintf(stderr, "%s.%s ", pnet[i].schema[k], pnet[i].tables[k]);
		if( pnet[i].inout[0])
			fprintf(stderr, " --> ");
		for (k = 0; k < MAXSTREAMS && pnet[i].schema[k]; k++)
		if( pnet[i].inout[k] == STREAM_OUT)
			fprintf(stderr, "%s.%s ", pnet[i].schema[k], pnet[i].tables[k]);
		if (pnet[i].error)
			fprintf(stderr, " errors:%s", pnet[i].error);
		fprintf(stderr, "\n");
	}
	(void) ret;
	return MAL_SUCCEED;
}

/*
 * The PetriNet scheduler lives in an separate thread.
 * It cycles through all transition nodes, hunting for paused queries that can fire.
 * The current policy is a simple round-robin. Later we will
 * experiment with more advanced schemes, e.g., priority queues.
 *
 * During each cycle step we first enable the transformations.
 *
 * Locking the streams is necessary to avoid concurrent changes.
 * Using a fixed order over the basket table, ensure no deadlock, but may render queries never to execute.
 */
static void
CQexecute( Client cntxt, int idx)
{
	CQnode *node= pnet+ idx;
	str msg;

	if( pnstatus != CQRUNNING)
		return;
	// first grab exclusive access to all streams.

#ifdef DEBUG_CQUERY
	fprintf(stderr, "#cquery.execute %s.%s locked\n",node->mod, node->fcn);
	fprintFunction(stderr, node->mb, 0, LIST_MAL_NAME | LIST_MAL_VALUE  | LIST_MAL_MAPI);
#endif

	msg = runMALsequence(cntxt, node->mb, 1, 0, node->stk, 0, 0);
	if( msg != MAL_SUCCEED)
		pnet[idx].error = msg;

	// release all locks held
#ifdef DEBUG_CQUERY
		fprintf(stderr, "#cquery.execute %s.%s finised %s\n", node->mod, node->fcn, (msg?msg:""));
#endif
	MT_lock_set(&ttrLock);
	if( node->status != CQPAUSE)
		node->status = CQWAIT;
	MT_lock_unset(&ttrLock);
}

static void
CQscheduler(void *dummy)
{
	int i, j;
	int k = -1;
	int pntasks;
	int delay = cycleDelay;
	Client cntxt = (Client) dummy;
	str msg = MAL_SUCCEED;
	lng t, now;
	BAT *claimed[MAXSTREAMS];
	BAT *b;

#ifdef DEBUG_CQUERY
	fprintf(stderr, "#cquery.scheduler started\n");
#endif
		
	CQinit = 1;
	MT_lock_set(&ttrLock);
	pnstatus = CQRUNNING; // global state 
	MT_lock_unset(&ttrLock);

	while( pnstatus != CQSTOP  && ! GDKexiting()){
		/* Determine which continuous queries are eligble to run.
  		   Collect latest statistics, note that we don't need a lock here,
		   because the count need not be accurate to the usec. It will simply
		   come back. We also only have to check the places that are marked
		   non empty. You can only trigger on empty baskets using a heartbeat */
		memset((void*) claimed, 0, sizeof(claimed));
		now = GDKusec();
		pntasks=0;
		MT_lock_set(&ttrLock); // analysis should be done with exclusive access
		for (k = i = 0; i < pnettop; i++) 
		if ( pnet[i].status == CQWAIT ){
			pnet[i].enabled = pnet[i].error == 0 && (pnet[i].cycles > 0 || pnet[i].cycles == int_nil);

			/* Queries are triggered by the heartbeat or  all window constraints */
			/* A heartbeat in combination with a window constraint is ambiguous */
			/* At least one constraint should be set */
			if( pnet[i].beats == lng_nil && pnet[i].bats[0] == 0)
				pnet[i].enabled = 0;

			if( pnet[i].enabled && pnet[i].beats > 0){
				if( pnet[i].run != lng_nil ) {
					pnet[i].enabled = now >= pnet[i].run + pnet[i].beats;
#ifdef DEBUG_CQUERY_SCHEDULER
					fprintf(stderr,"#beat %s.%s  "LLFMT"("LLFMT") %s\n", pnet[i].mod, pnet[i].fcn, 
						pnet[i].run + pnet[i].beats, now, (pnet[i].enabled? "enabled":"disabled"));
#endif
					}
				}

			/* check if all input baskets are available */
			for (j = 0; pnet[i].enabled && (b = pnet[i].bats[j]); j++)
				/* consider execution only if baskets are properly filled */
				if ( pnet[i].inout[j] == STREAM_IN && (BUN) pnet[i].window[j] > BATcount(b)){
					pnet[i].enabled = 0;
					break;
				} 

			/* check availability of all stream baskets */
			for (j = 0; pnet[i].enabled && (b = pnet[i].bats[j]); j++){
				for(k=0; claimed[k]; k++)
					if(claimed[k] == b)
						pnet[i].enabled = 0;
#ifdef DEBUG_CQUERY_SCHEDULER
						fprintf(stderr, "#cquery: %s.%s,disgarded \n", pnet[i].mod, pnet[i].fcn);
#endif
						break;
					}
				if (pnet[i].enabled && claimed[k] == 0){
					claimed[k] = b;
			}

#ifdef DEBUG_CQUERY_SCHEDULER
			if( pnet[i].enabled)
				fprintf(stderr, "#cquery: %s.%s enabled \n", pnet[i].mod, pnet[i].fcn);
#endif
			pntasks += pnet[i].enabled;
		}
		(void) fflush(stderr);
		MT_lock_unset(&ttrLock); 
#ifdef DEBUG_CQUERY_SCHEDULER
		if( pntasks)
			fprintf(stderr, "#Transitions %d enabled:\n",pntasks);
#else 
		(void) pntasks;
#endif
		if( pnstatus == CQSTOP)
			continue;

		/* Execute each enabled transformation */
		/* Tricky part is here a single stream used by multiple transitions */
		for (i = 0; i < pnettop; i++) 
		if( pnet[i].enabled){
#ifdef DEBUG_CQUERY
			fprintf(stderr, "#Run transition %s.%s cycle=%d \n", pnet[i].mod, pnet[i].fcn, pnet[i].cycles);
#endif

			t = GDKusec();
			// Fork MAL execution thread 
			CQexecute(cntxt, i);
/*
				if (MT_create_thread(&pnet[i].tid, CQexecute, (void*) (pnet+i), MT_THR_JOINABLE) < 0){
					msg= createException(MAL,"petrinet.scheduler","Can not fork the thread");
				} else
*/
			if( pnet[i].cycles != int_nil && pnet[i].cycles > 0)
				pnet[i].cycles--;
			pnet[i].run = now;				/* last executed */
			pnet[i].time = GDKusec() - t;   /* keep around in microseconds */
			(void) MTIMEcurrent_timestamp(&pnet[i].seen);
			pnet[i].enabled = 0;
			CQentry(i);
			if (msg != MAL_SUCCEED ){
				char buf[BUFSIZ];
				if (pnet[i].error == NULL) {
					snprintf(buf, BUFSIZ - 1, "Query %s.%s failed:%s", pnet[i].mod, pnet[i].fcn,msg);
					pnet[i].error = GDKstrdup(buf);
				} else
					GDKfree(msg);
			}
			delay = cycleDelay;
		}
		/* after one sweep all threads should be released */
/*
		for (m = 0; m < k; m++)
		if(pnet[enabled[m]].tid){
#ifdef DEBUG_CQUERY
			fprintf(stderr, "#Terminate query thread %s limit %d \n", pnet[enabled[m]].fcnname, pnet[enabled[m]].limit);
#endif
			MT_join_thread(pnet[enabled[m]].tid);
		}

#ifdef DEBUG_CQUERY
		if (pnstatus == CQRUNNING && cycleDelay) MT_sleep_ms(cycleDelay);  
#endif
		MT_sleep_ms(CQDELAY);  
*/
		/* we should actually delay until the next heartbeat or insertion into the streams */
		if (pntasks == 0  || pnstatus == CQPAUSE) {
#ifdef DEBUG_CQUERY
			fprintf(stderr, "#cquery.scheduler paused\n");
#endif
			if( pnettop == 0)
				break;
			MT_sleep_ms(delay);  
			if( delay < 20 * cycleDelay)
				delay *= 1.2;
		} 
	}
#ifdef DEBUG_CQUERY
	fprintf(stderr, "#cquery.scheduler stopped\n");
#endif
	cntxt->fdin = 0;
	cntxt->fdout = 0;
	//MCcloseClient(cntxt); to be checked, removes too much
	pnstatus = CQINIT;
	CQinit = 0;
}

str
CQstartScheduler(void)
{
	Client cntxt;
	MT_Id pid;
	//stream *fin, *fout;

#ifdef DEBUG_CQUERY
	fprintf(stderr, "#Start CQscheduler\n");
#endif
	MT_lock_init( &ttrLock, "cqueryLock");
/*
	fin =  open_rastream("cquery_in");
	if( fin == NULL)
		throw(MAL, "cquery.startScheduler","Could not create input file");
	fout =  open_wastream("cquery_out");
	if( fout == NULL)
		throw(MAL, "cquery.startScheduler","Could not create output file");
	cntxt = MCinitClient(0,bstream_create(fin,0),fout);
*/
	cntxt = MCinitClient(0,0,0);
	if( cntxt == NULL)
		throw(MAL, "cquery.startScheduler","Could not initialize CQscheduler");
	if( SQLinitClient(cntxt) != MAL_SUCCEED)
		throw(MAL, "cquery.startScheduler","Could not initialize CQscheduler");

	if (pnstatus== CQINIT && MT_create_thread(&pid, CQscheduler, (void*) cntxt, MT_THR_JOINABLE) != 0){
#ifdef DEBUG_CQUERY
		fprintf(stderr, "#Start CQscheduler failed\n");
#endif
		throw(MAL, "cquery.startScheduler", "cquery creation failed");
	}
	(void) pid;
	return MAL_SUCCEED;
}

