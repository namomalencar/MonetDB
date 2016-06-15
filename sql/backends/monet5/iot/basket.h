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

#ifndef _BASKETS_
#define _BASKETS_

#include "monetdb_config.h"
#include "mal.h"
#include "mal_interpreter.h"
#include "sql.h"
#include "iot.h"

/* #define _DEBUG_DATACELL     debug this module */
#define BSKTout GDKout
#define MAXBSKT 64

typedef struct{
	str schema_name;	/* schema for the basket */
	str table_name;	/* table that represents the basket */
	sql_schema *schema;
	sql_table *table;
	str *cols;

	int threshold ; /* bound to determine scheduling eligibility */
	BUN winsize, winstride; /* sliding window operations */
	lng timeslice, timestride; /* temporal sliding window, determined by first temporal component */
	lng heartbeat;	/* milliseconds delay between actions */
	BUN count;	/* number of events available in basket */

	/* statistics */
	int status;		/* (DE)ACTIVATE */
	timestamp seen;
	int events; /* total number of events grabbed */
	int cycles; 
	/* collected errors */
	BAT *errors;
	/* concurrency control between petrinet/{receptor,emitter} */
	MT_Lock lock;
	MT_Id pid;
	/* input/output destinations */
	str source;
} *Basket, BasketRec;


/* individual streams can be paused and restarted */
#define BSKTWAIT  		 1  /* waiting for new data*/
#define BSKTFILLED       2	/* some data available */

iot_export BasketRec *baskets;

iot_export str BSKTbind(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTregisterInternal(Client cntxt, MalBlkPtr mb, str sch, str tbl);
iot_export str BSKTregister(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export int BSKTlocate(str sch, str tbl);
iot_export str BSKTdump(void *ret);

iot_export str BSKTthreshold(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTheartbeat(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTwindow(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTreset(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTsettumble(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTtumble(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTcommit(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);

iot_export str BSKTtable( Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTtableerrors(bat *nmeId, bat *errorId);
iot_export str BSKTfinish( Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);

//iot_export str BSKTnewbasket(sql_schema *s, sql_table *t);
iot_export str BSKTdrop(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);

iot_export int BSKTlocate(str sch, str tbl);
iot_export int BSKTunlocate(str sch, str tbl);
iot_export int BSKTlocate(str sch, str tbl);
iot_export str BSKTappend(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTimportInternal(Client cntxt, int bskt);
iot_export str BSKTimport(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTerror(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTlock(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
iot_export str BSKTunlock(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);

#endif
