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
 * Copyright August 2008-2014 MonetDB B.V.
 * All Rights Reserved.
 */

#include "monetdb_config.h"
#include <sql_rdf_jgraph.h>
#include <rdf.h>
#include <rel_dump.h>
#include <rel_select.h>
#include <rdflabels.h>
#include <rel_exp.h>
#include <sql_rdf.h>
#include <rdfschema.h>

/*
static
void addRelationsToJG(mvc *sql, sql_rel *rel, int depth, jgraph *jg){
	
	char *r = NULL;

	if (!rel)
		return;

	if (rel_is_ref(rel)) {
		(void); 
	}

	switch (rel->op) {
	case op_basetable: {
	} 	break;
	case op_table:
		break;
	case op_ddl:
		break;
	case op_join: 
	case op_left: 
	case op_right: 
	case op_full: 
	case op_apply: 
	case op_semi: 
	case op_anti: 
	case op_union: 
	case op_inter: 
	case op_except: 
		r = "join";
		if (rel->op == op_left)
			r = "left outer join";
		else if (rel->op == op_right)
			r = "right outer join";
		else if (rel->op == op_full)
			r = "full outer join";
		else if (rel->op == op_apply) {
			r = "apply";
			if (rel->flag == APPLY_JOIN)
				r = "apply join";
			else if (rel->flag == APPLY_LOJ)
				r = "apply left outer join";
			else if (rel->flag == APPLY_EXISTS)
				r = "apply exists";
			else if (rel->flag == APPLY_NOTEXISTS)
				r = "apply not exists";
		}
		else if (rel->op == op_semi)
			r = "semijoin";
		else if (rel->op == op_anti)
			r = "antijoin";
		else if (rel->op == op_union)
			r = "union";
		else if (rel->op == op_inter)
			r = "intersect";
		else if (rel->op == op_except)
			r = "except";
		else if (!rel->exps && rel->op == op_join)
			r = "crossproduct";
		print_indent(sql, fout, depth);
		if (need_distinct(rel))
			mnstr_printf(fout, "distinct ");
		mnstr_printf(fout, "%s (", r);
		if (rel_is_ref(rel->l)) {
			int nr = find_ref(refs, rel->l);
			print_indent(sql, fout, depth+1);
			mnstr_printf(fout, "& REF %d ", nr);
		} else
			rel_print_(sql, fout, rel->l, depth+1, refs);
		mnstr_printf(fout, ",");
		if (rel_is_ref(rel->r)) {
			int nr = find_ref(refs, rel->r);
			print_indent(sql, fout, depth+1);
			mnstr_printf(fout, "& REF %d  ", nr);
		} else
			rel_print_(sql, fout, rel->r, depth+1, refs);
		print_indent(sql, fout, depth);
		mnstr_printf(fout, ")");
		exps_print(sql, fout, rel->exps, depth, 1, 0);
		break;
	case op_project:
	case op_select: 
	case op_groupby: 
	case op_topn: 
	case op_sample: 
		r = "project";
		if (rel->op == op_select)
			r = "select";
		if (rel->op == op_groupby)
			r = "group by";
		if (rel->op == op_topn)
			r = "top N";
		if (rel->op == op_sample)
			r = "sample";
		print_indent(sql, fout, depth);
		if (rel->l) {
			if (need_distinct(rel))
				mnstr_printf(fout, "distinct ");
			mnstr_printf(fout, "%s (", r);
			if (rel_is_ref(rel->l)) {
				int nr = find_ref(refs, rel->l);
				print_indent(sql, fout, depth+1);
				mnstr_printf(fout, "& REF %d ", nr);
			} else
				rel_print_(sql, fout, rel->l, depth+1, refs);
			print_indent(sql, fout, depth);
			mnstr_printf(fout, ")");
		}
		if (rel->op == op_groupby)  // group by columns 
			exps_print(sql, fout, rel->r, depth, 1, 0);
		exps_print(sql, fout, rel->exps, depth, 1, 0);
		if (rel->r && rel->op == op_project) // order by columns 
			exps_print(sql, fout, rel->r, depth, 1, 0);
		break;
	default:
		assert(0);
	}
}
*/



static 
str create_abstract_table(mvc *c){
	sql_table *tbl = NULL;
	str schema = "rdf"; 
	sql_schema *sch = NULL; 

	if ((sch = mvc_bind_schema(c, schema)) == NULL){
	 	throw(SQL, "sql_rdf_jgraph", "3F000!schema missing");
	}

	if ((tbl = mvc_bind_table(c, sch, tbl_abstract_name)) == NULL){
		sql_subtype tpe; 
		sql_find_subtype(&tpe, "oid", 31, 0);

		tbl = mvc_create_table(c, sch, tbl_abstract_name, tt_table, 0, SQL_PERSIST, 0, 3);
		mvc_create_column(c, tbl, "id", &tpe);
		assert(tbl != NULL); 
	}

	return MAL_SUCCEED; 
}

static
str add_abstract_column(mvc *c, str cname){
	
	sql_table *tbl = NULL;
	sql_schema *sch = NULL; 
	str schema = "rdf"; 
	sql_subtype tpe; 
	sql_column *col = NULL; 

	(void) col; 

	sql_find_subtype(&tpe, default_abstbl_col_type, 100, 0);

	if ((sch = mvc_bind_schema(c, schema)) == NULL){
	 	throw(SQL, "sql_rdf_jgraph", "3F000!schema missing");
	}

	if ((tbl = mvc_bind_table(c, sch, tbl_abstract_name)) == NULL){
	 	throw(SQL, "sql_rdf_jgraph", "tblabstract has not been created\n");
	}
	
	if ((col =  mvc_bind_column(c, tbl, cname)) == NULL){
		mvc_create_column(c, tbl, cname, &tpe); 
	}
	else
		printf("Column %s has already created in abstract table\n", cname); 

	return MAL_SUCCEED; 
}


static 
int is_basic_pattern(sql_rel *r){
	if (r->op != op_select && r->op != op_basetable){	//r->op != op_table is removed
		return 0; 
	}

	if (r->op == op_select){
		assert (r->l);
		if (((sql_rel *)r->l)->op == op_basetable){
			return 1; 
		}
		return 0; 
	}

	return 1; 

}

static 
void exps_print_ext(mvc *sql, list *exps, int depth, char *prefix){
	
	size_t pos;
	size_t nl = 0;
	size_t len = 0, lastpos = 0;

	stream *fd = sql->scanner.ws;
	stream *s;
	buffer *b = buffer_create(2000); /* hopefully enough */
	if (!b)
		return; /* signal somehow? */
	s = buffer_wastream(b, "SQL Plan");
	if (!s) {
		buffer_destroy(b);
		return; /* signal somehow? */
	}

	exps_print(sql, s, exps, depth, 1, 0);
	
	mnstr_printf(s, "\n");

	/* count the number of lines in the output, skip the leading \n */
	for (pos = 1; pos < b->pos; pos++) {
		if (b->buf[pos] == '\n') {
			nl++;
			if (len < pos - lastpos)
				len = pos - lastpos;
			lastpos = pos + 1;
		}
	}
	b->buf[b->pos - 1] = '\0';  /* should always end with a \n, can overwrite */

	mnstr_printf(fd, "%s \n", b->buf + 1 /* omit starting \n */);
	printf("%s %s\n", prefix, b->buf + 1 /* omit starting \n */);
	mnstr_close(s);
	mnstr_destroy(s);
	buffer_destroy(b);
}

static
void printRel_JGraph(jgraph *jg, mvc *sql){
	int i; 
	jgnode *tmpnode; 
	jgedge *tmpedge; 
	char tmp[50];
	printf("---- Join Graph -----\n"); 
	for (i = 0; i  < jg->nNode; i++){
		tmpnode = jg->lstnodes[i]; 
		//printf("Node %d: ", i); 
		sprintf(tmp, "Node %d: [Pattern: %d] ", i, tmpnode->patternId); 
		//mnstr_printf(sql->scanner.ws, "Node %d: ", i); 
		exps_print_ext(sql, ((sql_rel *) tmpnode->data)->exps, 0, tmp); 
		tmpedge = tmpnode->first; 
		while (tmpedge != NULL){
			assert(tmpedge->from == tmpnode->vid); 
			printf(" %d", tmpedge->to); 
			tmpedge = tmpedge->next; 
		}
		printf("\n"); 
		//mnstr_printf(sql->scanner.ws, "\n"); 
	}
	printf("---------------------\n"); 
}

/*
 * Algorithm for adding sql rels to Join Graph
 *
 * 1. We consider joins that are directed connected to each other
 * are belonging to one subgraph of join. 
 * E.g., For join1->join2->project->join3->join4->join5, join1, join2
 * belong to one subgraph, join3,4,5 belong to one subgraph
 *
 * 2. We go from the top sql_rel
 *
 * */
static
void addRelationsToJG(mvc *c, sql_rel *rel, int depth, jgraph *jg, int new_subjg, int *subjgId){
	int tmpvid =-1; 

	switch (rel->op) {
		case op_left:
		case op_right:
		case op_join:
			if (rel->op == op_left || rel->op == op_right) printf("[Outter join]\n");
			else printf("[join]\n"); 

			printf("--- Between %s and %s ---\n", op2string(((sql_rel *)rel->l)->op), op2string(((sql_rel *)rel->r)->op) );		
			
			if (new_subjg){ 	//The new subgraph flag is set
				*subjgId = *subjgId + 1; 
			}

			addRelationsToJG(c, rel->l, depth+1, jg, 0, subjgId);
			addRelationsToJG(c, rel->r, depth+1, jg, 0, subjgId);

			break; 
		case op_select: 
			 printf("[select]\n");
			if (is_basic_pattern(rel)){
				printf("Found a basic pattern\n");
				//rel_print(c, rel, 0);
				addJGnode(&tmpvid, jg, (sql_rel *) rel, *subjgId); 
			}
			else{	//This is the connect to a new join sg
				addRelationsToJG(c, rel->l, depth+1, jg, 1, subjgId);
			}
			break; 
		case op_basetable:
			printf("[Base table]\n");		
			addJGnode(&tmpvid, jg, (sql_rel *) rel, *subjgId);
			//rel_print(c, rel, 0);
			break;
		default:
			printf("[%s]\n", op2string(rel->op)); 
			if (rel->l) 
				addRelationsToJG(c, rel->l, depth+1, jg, 1, subjgId); 
			if (rel->r)
				addRelationsToJG(c, rel->r, depth+1, jg, 1, subjgId); 
			break; 
			
	}

}

/*
 * TODO: Should we use the function rel_name() from rel_select.c 
 * Consider removing this function
 * */
static
char *get_relname_from_basetable(sql_rel *rel){

	sql_exp *tmpexp;
	char *rname = NULL; 
	list *tmpexps; 

	assert(rel->op == op_basetable); 
	tmpexps = rel->exps;
	if (tmpexps){
		node *en; 
		
		rname = ((sql_exp *) tmpexps->h->data)->rname; 
		//For verifying that there is 
		//only one relation name
		for (en = tmpexps->h; en; en = en->next){
			tmpexp = (sql_exp *) en->data; 
			assert(tmpexp->type == e_column);
			printf("[Table] %s -> [Column] %s", tmpexp->rname, tmpexp->name);
			assert(strcmp(rname, tmpexp->rname) == 0); 
		}
	}
	
	printf("rname %s vs rname %s from rel_name fucntion", rname, rel_name(rel));
	return rname; 

}



/*
 * Get the name of the relation of each JG node
 * */
/*
static 
char *getNodeRel(jgnode *node){
	sql_rel *tmprel = (sql_rel *) node->data;
	assert(tmprel != NULL)
	switch (tmprel->op){
		case op_basetable: 

			break; 
		case op_select:
			break;
		default: 
			assert(0); 
	}

}
*/

static 
nMap *create_nMap(int maxnum){
	nMap *nm; 
	nm = (nMap*) malloc(sizeof(nMap));
	nm->lmap = BATnew(TYPE_void, TYPE_str, maxnum, TRANSIENT); 
	nm->rmap = BATnew(TYPE_void, TYPE_int, maxnum, TRANSIENT); 
	
	return nm; 
}
static
void free_nMap(nMap *nm){
	
	BBPunfix(nm->lmap->batCacheid); 
	BBPunfix(nm->rmap->batCacheid);
	free(nm); 
}
static 
void add_to_nMap(nMap *nm, str s, int *id){
	BUN bun; 

	bun = BUNfnd(nm->lmap,(ptr) (str)s);
	if (bun == BUN_NONE){
		BUNappend(nm->lmap, s, TRUE); 
		BUNappend(nm->rmap, id, TRUE); 
		printf("Add rname %s | %d to nmap\n", s, *id); 
	}
	else{
	
		printf("This should not happen\n");
		assert(0); 
	}
}

static 
int rname_to_nodeId(nMap *nm, str rname){
	int *id; 
	BUN bun; 
	bun = BUNfnd(nm->lmap,rname);

	if (bun == BUN_NONE){
		printf("Rel %s is not found in nmap \n", rname); 
		return -1; 
	}
	else{
		id = (int *) Tloc(nm->rmap, bun);  		
	}
	
	return *id; 
}


static 
void add_relNames_to_nmap(jgraph *jg, nMap *nm){
			
	jgnode *tmpnode; 
	//jgedge *tmpedge; 
	sql_rel *tmprel; 
	int i; 
	for (i = 0; i  < jg->nNode; i++){
		tmpnode = jg->lstnodes[i]; 
		tmprel = (sql_rel *) tmpnode->data; 
		
		if (tmprel->op == op_basetable){
			str s = get_relname_from_basetable(tmprel); 
			printf("[Node %d --> Table] %s\n", i, s);
			add_to_nMap(nm, s, &i); 

		}
		else if (tmprel->op == op_select){
			str s; 
			assert(((sql_rel *)tmprel->l)->op == op_basetable); //Only handle the case 
									    //when selecting from base_table	
			s = get_relname_from_basetable((sql_rel *)tmprel->l); 	
			printf("[Node %d -->Table from select] %s\n",i, s); 
			add_to_nMap(nm, s, &i);
		}
	}

	//Test the rname_to_id
	for (i = (jg->nNode - 1); i  >= 0; i--){
		tmpnode = jg->lstnodes[i];
		tmprel = (sql_rel *) tmpnode->data;
		if (tmprel->op == op_basetable){
			str s = get_relname_from_basetable(tmprel);
			printf("Get nodeId for %s from nmap: %d\n", s, rname_to_nodeId(nm, s)); 
		}
		else if (tmprel->op == op_select){
			str s = get_relname_from_basetable((sql_rel *)tmprel->l);
			printf("Get nodeId for %s from nmap: %d\n", s, rname_to_nodeId(nm, s));
		}

	}
}

static
void get_jp(str pred1, str pred2, JP *jp){
	
	if (strcmp(pred1, "s")==0 && strcmp(pred2, "s")==0){
			*jp = JP_S; 
	
	} else if (strcmp(pred1, "o")==0 && strcmp(pred2, "o")==0){
			*jp = JP_O; 
	} else
		*jp = JP_NAV; 
	
}

/*
 * Input: sql_rel with op == op_join, op_left or op_right
 * */
static
void _add_join_edges(jgraph *jg, sql_rel *rel, nMap *nm, char **isConnect){

	sql_exp *tmpexp;
	list *tmpexps; 

	assert(rel->op == op_join || rel->op == op_left || rel->op == op_right); 
	tmpexps = rel->exps;
	if (tmpexps){
		node *en; 

		for (en = tmpexps->h; en; en = en->next){
			sql_exp *l; 
			sql_exp *r; 
			tmpexp = (sql_exp *) en->data;
			if(tmpexp->type == e_cmp) {
				int to, from; 

				l = tmpexp->l; 
				r = tmpexp->r; 
				assert(l->type == e_column);
				assert(r->type == e_column); 
				printf("Join: [Table]%s.[Column]%s == [Table]%s.[Column]%s \n", l->rname, l->name, r->rname, r->name);
				from = rname_to_nodeId(nm, l->rname); 
				to = rname_to_nodeId(nm, r->rname); 
				printf("Node %d to Node %d\n", from, to); 
				if (isConnect[from][to] == 0){
					JP tmpjp = JP_NAV; 
					get_jp(l->name, r->name, &tmpjp); 
					printf("Join predicate = %d\n", tmpjp); 
					addJGedge(from, to, rel->op, jg, rel, tmpjp);
					isConnect[from][to] = 1;  
				}
				else{
					JP tmpjp = JP_NAV;
					printf("This edge is created \n"); 
					get_jp(l->name, r->name, &tmpjp);
					if (1) update_edge_jp(jg, from, to, tmpjp); 
					printf("Updated join predicate = %d\n", tmpjp); 
				}
			} else if (tmpexp->type == e_atom){
				//Only handle the case of 1
				//TODO: Handle other cases
				int tmpCond; 
				int from, to; 
				assert(tmpexp->l); 
				assert(atom_type(tmpexp->l)->type->localtype != TYPE_ptr);
				tmpCond = (int) atom_get_int(tmpexp->l); 
				//printf("Atom value %d \n",tmpCond);
				if (tmpCond == 1){
					printf("Join (condition 1) between %s and %s\n", rel_name((sql_rel*) rel->l), rel_name((sql_rel *)rel->r)); 
					from = rname_to_nodeId(nm,  rel_name((sql_rel*) rel->l));
					to = rname_to_nodeId(nm,  rel_name((sql_rel*) rel->r));	
					addJGedge(from, to, rel->op, jg, rel, JP_NAV);
				}
				continue; 
			} else {
				printf("This tmpexp->type == %d has not been handled yet\n", tmpexp->type); 
				assert(0); 
			}

		}
		printf("\n\n\n"); 
	}

}


static
void addJoinEdgesToJG(mvc *c, sql_rel *rel, int depth, jgraph *jg, int new_subjg, int *subjgId, nMap *nm, char **isConnect){
	//int tmpvid =-1; 

	switch (rel->op) {
		case op_left:
		case op_right:
		case op_join:
			if (new_subjg){ 	//The new subgraph flag is set
				*subjgId = *subjgId + 1; 
			}
			addJoinEdgesToJG(c, rel->l, depth+1, jg, 0, subjgId, nm, isConnect);
			addJoinEdgesToJG(c, rel->r, depth+1, jg, 0, subjgId, nm, isConnect);
			// Get the node Ids from 			
			_add_join_edges(jg, rel, nm, isConnect); 
			break; 
		case op_select: 
			if (is_basic_pattern(rel)){
				//printf("Found a basic pattern\n");
				//rel_print(c, rel, 0);
			}
			else{	//This is the connect to a new join sg

				//if is_join(((sql_rel *)rel->l)->op) printf("Join graph will be connected from here\n"); 
				addJoinEdgesToJG(c, rel->l, depth+1, jg, 1, subjgId, nm, isConnect);
			}
			break; 
		case op_basetable:
			break;
		default:		//op_project, topn,...
			if (rel->l){
				//if is_join(((sql_rel *)rel->l)->op) printf("Join graph will be connected from here\n"); 
				addJoinEdgesToJG(c, rel->l, depth+1, jg, 1, subjgId, nm, isConnect); 
			}
			if (rel->r){
				//if is_join(((sql_rel *)rel->l)->op) printf("Join graph will be connected from here\n"); 
				addJoinEdgesToJG(c, rel->r, depth+1, jg, 1, subjgId, nm, isConnect); 
			}
			break; 
			
	}

}

static 
char** createMatrix(int num, char initValue){
	int i, j; 
	char **m; 

	m = (char **)malloc(sizeof(char*) * num);
	for (i = 0; i < num; i++){
		m[i] = (char *)malloc(sizeof(char) * num);
		for (j = 0; j < num; j++){
			m[i][j] = initValue; 
		}
	}

	return m; 
}
static 
void freeMatrix(int num, char **m){
	int i; 
	for (i = 0; i < num; i++){
		free(m[i]);
	}
	free(m); 
}

static 
void _detect_star_pattern(jgraph *jg, jgnode *node, int pId){
	//Go through all edges of the node
	//If the edge has join predicate JP_S
	//and the to_node does not belong to
	//any pattern, add to the current pattern
	jgedge *tmpedge; 
	tmpedge = node->first; 
	while (tmpedge != NULL){
		if (tmpedge->jp == JP_S){
			jgnode *tonode = jg->lstnodes[tmpedge->to]; 
			if (tonode->patternId == -1){
				tonode->patternId = pId; 
				_detect_star_pattern(jg, tonode, pId); 
			} 
		}
		tmpedge = tmpedge->next; 
	}
}

/*
 * Init property list (columns) extracted from
 * a star pattern
 * */
static 
spProps *init_sp_props(int num){
	int i; 
	spProps* spprops = NULL; 
	spprops = (spProps*) GDKmalloc(sizeof (spProps) ); 
	spprops->num = num; 
	spprops->lstProps = (char **) GDKmalloc(sizeof(char *) * num); 
	spprops->lstPropIds = (oid *) GDKmalloc(sizeof(oid) * num); 

	for (i = 0; i < num; i++){
		spprops->lstProps[i] = NULL; 
		spprops->lstPropIds[i] = BUN_NONE; 
	}
	spprops->lstPOs = (sp_po *) GDKmalloc(sizeof(sp_po) * num); 

	return spprops; 
}

static
void add_props_to_spprops(spProps *spprops, int idx, sp_po po, char *col){
	oid id; 

	spprops->lstProps[idx] = GDKstrdup(col); 

	//Get propId, assuming the tokenizer is open already 
	TKNRstringToOid(&id, & (spprops->lstProps[idx])); 
	spprops->lstPropIds[idx] = id;  
	
	spprops->lstPOs[idx] = po; 

}

static
void print_spprops(spProps *spprops){
	int i; 
	
	printf("List of properties: \n");
	for (i = 0; i < spprops->num; i++){
		printf("%s (Id: "BUNFMT ")\n" ,spprops->lstProps[i], spprops->lstPropIds[i]);	
	}
	printf("\n"); 
}

static 
void free_sp_props(spProps *spprops){
	int i; 
	for (i = 0; i < spprops->num; i++){
		if (spprops->lstProps[i]) GDKfree(spprops->lstProps[i]); 
	}
	GDKfree(spprops->lstProps); 
	GDKfree(spprops->lstPropIds);
	GDKfree(spprops->lstPOs); 
	GDKfree(spprops); 
}

/*
 * Get column name from exp of p in a sql_rel
 * Example: s12_t0.p = oid[sys.rdf_strtoid(char(67) "<http://www/product>")]
 * ==> column name = product.
 * This column must match with the table/column created from Characteristic Sets.
 * */

static
void get_col_name_from_p (char **col, char *p){
	getPropNameShort(col, p);
}

/*
 * Modify the tablename.colname in an
 * expression. 
 *
 * E.g., s12_t0.o = oid[sys.rdf_strtoid(char(85) "<http://www/Product9>"]
 * will be convert to tbl1.p = oid[sys.rdf_strtoid(char(85) "<http://www/Product9>"]
 * */
static
void modify_exp_col(sql_allocator *sa, sql_exp *m_exp,  char *_rname, char *_name){
	sql_exp *e = NULL;
	sql_exp *ne = NULL;
	
	str rname = GDKstrdup(_rname); 
	str name = GDKstrdup(_name);

	//exp_setname(sa, e, rname, name); 
	assert(m_exp->type == e_cmp); 

	e = (sql_exp *)m_exp->l; 
	assert(e->type == e_column); 

 	ne = exp_column(sa, rname, name, exp_subtype(e), exp_card(e), has_nil(e), 0);

	m_exp->l = ne; 

	//TODO: Convert subtype to the type of new col
}

/*
 * From op_select sql_rel, get the condition on p and o which 
 * can indicate the column name of the abstract table
 * and the condition on that column
 *
 *
 * */
static
void get_prop_and_exps(mvc *c, sql_rel *r, char **prop, list *trans_exps){
	
	list *tmpexps = NULL; 
	char select_s = 0, select_p = 0, select_o = 0; 
	sql_allocator *sa = c->sa; 
	str col; 

	assert(r->op == op_select);
	assert(((sql_rel*)r->l)->op == op_basetable); 
	
	//Verify that this select function select all three columns s, p, o
		
	tmpexps = ((sql_rel*)r->l)->exps;
	if (tmpexps){
		node *en;
	
		for (en = tmpexps->h; en; en = en->next){
			sql_exp *tmpexp = (sql_exp *) en->data; 
			assert(tmpexp->type == e_column);
			if (strcmp(tmpexp->name, "s") == 0) select_s = 1; 
			if (strcmp(tmpexp->name, "p") == 0) select_p = 1; 
			if (strcmp(tmpexp->name, "o") == 0) select_o = 1; 
		}
	}

	assert(select_s && select_p && select_o);
	
	printf("Converting op_select in star pattern to sql_rel of abstract table\n"); 
	//Get the column name by checking exps of r
	
	tmpexps = r->exps;
	//First go through all exps to get p --> col name
	
	if (tmpexps){
		node *en;
		int num_p_cond = 0; 
	
		for (en = tmpexps->h; en; en = en->next){
			sql_exp *tmpexp = (sql_exp *) en->data; 
			list *lst = NULL; 

			assert(tmpexp->type == e_cmp); //TODO: Handle other exps for op_select
			
			//Example: [s12_t0.p = oid[sys.rdf_strtoid(char(67) "<http://www/product>")], s12_t0.o = oid[sys.rdf_strtoid(char(85) "<http://www/Product9>"]
			assert(((sql_exp *)tmpexp->l)->type == e_column); 

			//Check if the column name is p, then
			//extract the input property name
			if (strcmp(((sql_exp *)tmpexp->l)->name, "p") == 0){
				sql_exp *tmpexp2;
				node *tmpen; 
				str s;
				
				num_p_cond++; 
				assert(((sql_exp *)tmpexp->r)->type == e_convert);

				tmpexp = (sql_exp *) ((sql_exp *)tmpexp->r)->l;
				assert(tmpexp->type == e_func); 

				lst = tmpexp->l;
				
				//There should be only one parameter for the function which is the property name
				tmpen = lst->h; 
				tmpexp2 = (sql_exp *) tmpen->data;
							
				s = atom2string(c->sa, (atom *) tmpexp2->l); 
				*prop = GDKstrdup(s); 
				get_col_name_from_p (&col, s);
				printf("%s --> corresponding column %s\n", *prop,  col); 
				
				//In case the column name is not in the abstract table, add it
				add_abstract_column(c, col);

			} else{ 
				continue; 
			}


		}

		assert(num_p_cond == 1 && col != NULL); //Verify that there is only one p in this op_select sql_rel 
	}
	if (tmpexps){
		node *en;
		int num_o_cond = 0;
		int num_s_cond = 0; 
	
		for (en = tmpexps->h; en; en = en->next){
			sql_exp *tmpexp = (sql_exp *) en->data; 
			sql_exp *e = (sql_exp *)tmpexp->l; 

			assert(tmpexp->type == e_cmp); //TODO: Handle other exps for op_select
			assert(e->type == e_column); 

			if (strcmp(e->name, "p") == 0){
				continue; 

			} else if (strcmp(e->name, "o") == 0){
				sql_exp *m_exp = exp_copy(sa, tmpexp);
				modify_exp_col(sa, m_exp, tbl_abstract_name, col);
				
				//append this exp to list
				append(trans_exps, m_exp);
				num_o_cond++;

			} else if (strcmp(e->name, "s") == 0){
				sql_exp *m_exp = exp_copy(sa, tmpexp);
				modify_exp_col(sa, m_exp, tbl_abstract_name, col);

				//append this exp to list
				append(trans_exps, m_exp);
				num_s_cond++;
			} else{ 
				printf("The exp of other predicates (not s, p, o) is not handled\n"); 
			}


		}

	}

}


static
void tranforms_exps(mvc *c, sql_rel *r, list *trans_exps, int tblId){

	list *tmpexps = NULL; 
	sql_allocator *sa = c->sa; 
	char tmpcolname[100]; //TODO: Should we use char[]


	assert(r->op == op_select);
	assert(((sql_rel*)r->l)->op == op_basetable); 
	
	printf("Converting op_select in star pattern to sql_rel of abstract table\n"); 
	//Get the column name by checking exps of r
	
	tmpexps = r->exps;

	//First go through all exps to get p --> col name
	
	if (tmpexps){
		node *en;
		int num_p_cond = 0; 
	
		for (en = tmpexps->h; en; en = en->next){
			sql_exp *tmpexp = (sql_exp *) en->data; 
			list *lst = NULL; 

			assert(tmpexp->type == e_cmp); //TODO: Handle other exps for op_select
			
			//Example: [s12_t0.p = oid[sys.rdf_strtoid(char(67) "<http://www/product>")], s12_t0.o = oid[sys.rdf_strtoid(char(85) "<http://www/Product9>"]
			assert(((sql_exp *)tmpexp->l)->type == e_column); 

			//Check if the column name is p, then
			//extract the input property name
			if (strcmp(((sql_exp *)tmpexp->l)->name, "p") == 0){
				sql_exp *tmpexp2;
				node *tmpen; 
				str s;
				oid tmpPropId;
				int colIdx; 
				
				num_p_cond++; 
				assert(((sql_exp *)tmpexp->r)->type == e_convert);

				tmpexp = (sql_exp *) ((sql_exp *)tmpexp->r)->l;
				assert(tmpexp->type == e_func); 

				lst = tmpexp->l;
				
				//There should be only one parameter for the function which is the property name
				tmpen = lst->h; 
				tmpexp2 = (sql_exp *) tmpen->data;
							
				s = atom2string(c->sa, (atom *) tmpexp2->l); 

				TKNRstringToOid(&tmpPropId, &s);

				colIdx = getColIdx_from_oid(tblId, global_csset, tmpPropId);

				getColSQLname(tmpcolname, colIdx, -1, tmpPropId, global_mapi, global_mbat);

				printf("In transform %s --> corresponding column %s\n", s,  tmpcolname); 
				
				//In case the column name is not in the abstract table, add it
				//add_abstract_column(c, tmpcolname);

			} else{ 
				continue; 
			}


		}

		assert(num_p_cond == 1 && tmpcolname != NULL); //Verify that there is only one p in this op_select sql_rel 
	}

	if (tmpexps){
		node *en;
		int num_o_cond = 0;
		int num_s_cond = 0; 
	
		for (en = tmpexps->h; en; en = en->next){
			sql_exp *tmpexp = (sql_exp *) en->data; 
			sql_exp *e = (sql_exp *)tmpexp->l; 

			assert(tmpexp->type == e_cmp); //TODO: Handle other exps for op_select
			assert(e->type == e_column); 

			if (strcmp(e->name, "p") == 0){
				continue; 

			} else if (strcmp(e->name, "o") == 0){
				sql_exp *m_exp = exp_copy(sa, tmpexp);
				modify_exp_col(sa, m_exp, tbl_abstract_name, tmpcolname);
				
				//append this exp to list
				append(trans_exps, m_exp);
				num_o_cond++;

			} else if (strcmp(e->name, "s") == 0){
				sql_exp *m_exp = exp_copy(sa, tmpexp);
				modify_exp_col(sa, m_exp, tbl_abstract_name, tmpcolname);

				//append this exp to list
				append(trans_exps, m_exp);
				num_s_cond++;
			} else{ 
				printf("The exp of other predicates (not s, p, o) is not handled\n"); 
			}


		}

	}

}


/*
 * 
 * */
static 
void getTblName_from_spprops(str *tblname, int *rettbId, spProps *spprops){

	oid *lstprop = NULL; 	//list of distinct prop, sorted by prop ids
	int num; 		//number of of distinct sorted props
	int i,j; 
	int **tmptblId; 	//lists of tables corresonding to each prop
	int *count; 		//number of tables per prop	
	int *tblId; 		//list of matching tblId
	int numtbl = 0; 	

	get_sorted_distinct_set(spprops->lstPropIds, &lstprop, spprops->num, &num);
	
	tmptblId = (int **) malloc(sizeof(int *) * num); 
	count = (int *) malloc(sizeof(int) * num); 

	printf("Table Id for set of props [");
	for (i = 0; i < num; i++){
		Postinglist pl = get_p_postingList(global_p_propstat, lstprop[i]);
		tmptblId[i] = pl.lstIdx;
		count[i] = pl.numAdded; 
		printf("  " BUNFMT, lstprop[i]);
	}
	
	intersect_intsets(tmptblId, count, num, &tblId,  &numtbl);

	printf(" ] --> ");
	for (i = 0; i < numtbl; i++){
		char tblname[100];
		int tId = tblId[i];
		oid tblnameoid = global_csset->items[tId]->tblname; 

		*rettbId = tId; 

		getTblSQLname(tblname, i, -1,  tblnameoid, global_mapi, global_mbat);

		printf("  %d [Name of the table  %s]", tId, tblname);  

		//Get the corresponding column names in this table
		printf("\n--> Corresponding column names: \n");
		for (j = 0; j < num; j++){
			char tmpcolname[100];
			int colIdx = getColIdx_from_oid(tId, global_csset, lstprop[j]);
			getColSQLname(tmpcolname, colIdx, -1, lstprop[j], global_mapi, global_mbat);
			printf("Col %d: %s\n",j, tmpcolname);
		}
	}

	printf("\n"); 

	(void) tblname; 
}

static 	
sql_rel* _group_star_pattern(mvc *c, jgraph *jg, int *group, int nnode, int pId){
	sql_rel *rel = NULL; 
	int i; 
	char is_all_select = 1; 
	sql_allocator *sa = c->sa;
	spProps *spprops = NULL; 
	str tblname; 
	int tmptbId = -1; 

	list *trans_exps = NULL; //This transformed exps list contain exps list from op_select
				 //on the object
	list *trans_exps_real = NULL; 
	(void) jg; 

	printf("Group %d contain %d nodes: ", pId, nnode); 

	for (i = 0; i < nnode; i++){
		sql_rel *tmprel = (sql_rel*) (jg->lstnodes[group[i]]->data);
		printf(" %d ", group[i]); 
		rel_print(c, tmprel, 0); 
		if (tmprel->op != op_select) is_all_select = 0; 	
	}
	printf("\n"); 
	
	spprops = init_sp_props(nnode); 	
	trans_exps = new_exp_list(sa);
	trans_exps_real = new_exp_list(sa);

	//Convert to sql_rel of abstract table
	if (is_all_select){
		char tmp[50]; 
		for (i = 0; i < nnode; i++){
			str col; 
			sql_rel *tmprel = (sql_rel*) (jg->lstnodes[group[i]]->data);
			get_prop_and_exps(c, tmprel, &col, trans_exps); 
			add_props_to_spprops(spprops, i, NAV, col); 		
			GDKfree(col); 
		}
		
		sprintf(tmp, "[Pattern: %d] after grouping: ", pId); 
		exps_print_ext(c, trans_exps, 0, tmp);
	}

	
	print_spprops(spprops);

	getTblName_from_spprops(&tblname, &tmptbId, spprops);

	printf("Get real experessions from tableId %d\n", tmptbId);

	if (is_all_select){
		char tmp[50]; 
		for (i = 0; i < nnode; i++){
			sql_rel *tmprel = (sql_rel*) (jg->lstnodes[group[i]]->data);
			tranforms_exps(c, tmprel, trans_exps_real, tmptbId); 
		}
		
		sprintf(tmp, "[Real Pattern: %d] after grouping: ", pId); 
		exps_print_ext(c, trans_exps_real, 0, tmp);
	}

	if (is_all_select){
		sql_rel *m_rel = rel_copy(c->sa, (sql_rel*) (jg->lstnodes[group[0]]->data));
		for (i = 1; i < nnode; i++){
			sql_rel *tmprel = (sql_rel*) (jg->lstnodes[group[i]]->data);
			list *exps = tmprel->exps; 
			if (exps){
				node *en; 
				//add each exp to the new merged sql_rel 
				for(en = exps->h; en; en = en->next){
					rel_select_add_exp(c->sa, m_rel, en->data); 
				}
			}
			rel_print(c, m_rel, 0); 
		}
	}

	//TODO: Handle other cases. By now, we only handle 
	//the case where each sql_rel is a op_select. 
	


	free_sp_props(spprops);
	list_destroy(trans_exps);

	return rel; 
}

static 
void group_star_pattern(mvc *c, jgraph *jg, int numsp){

	int i; 
	int** group; //group of nodes in a same pattern
	int* nnode_per_group; 
	int* idx; 
	sql_rel** lstRels; 
	
	group = (int **)malloc(sizeof(int*) * numsp); 
	nnode_per_group = (int *) malloc(sizeof(int) * numsp);
	idx = (int *) malloc(sizeof(int) * numsp);

	lstRels = (sql_rel**) malloc(sizeof(sql_rel*) * numsp); 

	for (i = 0; i < numsp; i++){
		nnode_per_group[i] = 0;
		idx[i] = 0;
	}

	for (i = 0; i < jg->nNode; i++){
		jgnode *node = jg->lstnodes[i]; 
		assert(node->patternId < numsp); 
		nnode_per_group[node->patternId]++; 	
	}

	//Init for group
	for (i = 0; i < numsp; i++){
		group[i] = (int*) malloc(sizeof(int) * nnode_per_group[i]); 
	}

	//add nodeIds for each group
	
	for (i = 0; i < jg->nNode; i++){
		jgnode *node = jg->lstnodes[i];
		int spId = node->patternId; 
		group[spId][idx[spId]] = node->vid; 
		idx[spId]++; 
	}

	//Merge sql_rels in each group into one sql_rel
	for (i = 0; i < numsp; i++){
		lstRels[i] = _group_star_pattern(c, jg, group[i], nnode_per_group[i], i); 
	}

	//Free
	for (i = 0; i < numsp; i++){
		free(group[i]);
	}
	free(group); 
	free(nnode_per_group); 
	free(idx); 
}

static
void detect_star_pattern(jgraph *jg, int *numsp){
	
	int i; 
	int pId = -1; 
	int num = jg->nNode;

	for (i = 0; i < num; i++){
		jgnode *node = jg->lstnodes[i]; 
		if (node->patternId == -1){
			pId++;
			node->patternId = pId; 
			_detect_star_pattern(jg, node, pId); 	
		}
	}

	*numsp = pId + 1; 
}

void buildJoinGraph(mvc *c, sql_rel *r, int depth){
	//rel_print(c, r, 0);
	//Detect join between subject of triple table
	//
	// Go from the first sql_rel to the left and right of 
	// the sql_rel
	// If sql_rel has the op is op_join, check 
	// the exps (see rel_dump.c:264) in order to 
	// check from which table and condition 
	
	jgraph *jg; 
	nMap *nm = NULL; 
	int subjgId = -1; 
	int subjgId2 = -1;
	char **isConnect; //Matrix storing state whether two nodes are conneccted	
			  //In case of large sparse graph, this should not be used.
	int numsp = 0; 	  //Number of star pattern

	(void) c; 
	(void) r; 
	(void) depth; 
	jg = initJGraph(); 
	
	addRelationsToJG(c, r, depth, jg, 0, &subjgId); 
	
	nm = create_nMap(MAX_JGRAPH_NODENUMBER); 
	add_relNames_to_nmap(jg, nm); 

	isConnect = createMatrix(jg->nNode, 0); 

	addJoinEdgesToJG(c, r, depth, jg, 0, &subjgId2, nm, isConnect);

	detect_star_pattern(jg, &numsp); 

	printRel_JGraph(jg, c); 

	create_abstract_table(c);
	
	group_star_pattern(c, jg, numsp); 

	//Check global_csset
	print_simpleCSset(global_csset);   

	free_nMap(nm); 
	freeMatrix(jg->nNode, isConnect); 
	//printJGraph(jg); 

	freeJGraph(jg); 
	
	/*
	switch (r->op) {
		case op_join:
			if (((sql_rel *)r->l)->op == op_select && ((sql_rel *)r->r)->op == op_select){
				printf("POSSIBLE STAR PATTERN\n");		
			}
			else{
				if (((sql_rel *)r->l)->op != op_select) 
					buildJoinGraph(c, r->l, depth + 1);
				if (((sql_rel *)r->r)->op != op_select)
					buildJoinGraph(c, r->r, depth + 1);
			}
		case op_basetable:
			break; 	//Temporarily do nothign
		default:
			if (r->l) 
				buildJoinGraph(c, r->l, depth+1); 
			if (r->r)
				buildJoinGraph(c, r->r, depth+1); 
			
	}
	*/
}

