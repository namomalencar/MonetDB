#include "monetdb_config.h"
#include "gdk.h"
#include "gdk_arrays.h"
#include "math.h"
#include "mal_exception.h"
#include "mal_interpreter.h"

#include "calc_arrays.h"

static BAT* dimension2BAT(const gdk_analytic_dimension *dim) {
	BAT *dimBAT = BATnew(TYPE_void, dim->type, 3, TRANSIENT);

	if(!dimBAT)
		return NULL;

	BUNappend(dimBAT, dim->min, TRUE);
	BUNappend(dimBAT, dim->max, TRUE);
	BUNappend(dimBAT, dim->step, TRUE);

	return dimBAT;	
}

static gdk_analytic_dimension* BAT2dimension(BAT *dimBAT, int dimNum) {
	BATiter biter = bat_iterator(dimBAT);
	BUN currBun = BUNfirst(dimBAT);

	switch(BATttype(dimBAT)) {
		case TYPE_bte: { 
			bte min = *(bte*)BUNtail(biter, currBun);
			bte max = *(bte*)BUNtail(biter, currBun+1);
			bte step = *(bte*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_bte(dimNum, min, max, step);
		}
		case TYPE_sht: { 
			short min = *(short*)BUNtail(biter, currBun);
			short max = *(short*)BUNtail(biter, currBun+1);
			short step = *(short*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_sht(dimNum, min, max, step);
		}
		case TYPE_int: { 
			int min = *(int*)BUNtail(biter, currBun);
			int max = *(int*)BUNtail(biter, currBun+1);
			int step = *(int*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_int(dimNum, min, max, step);
		}
		case TYPE_wrd: { 
			wrd min = *(wrd*)BUNtail(biter, currBun);
			wrd max = *(wrd*)BUNtail(biter, currBun+1);
			wrd step = *(wrd*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_wrd(dimNum, min, max, step);
		}
		case TYPE_oid: { 
			oid min = *(oid*)BUNtail(biter, currBun);
			oid max = *(oid*)BUNtail(biter, currBun+1);
			oid step = *(oid*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_oid(dimNum, min, max, step);
		}
		case TYPE_lng: { 
			long min = *(long*)BUNtail(biter, currBun);
			long max = *(long*)BUNtail(biter, currBun+1);
			long step = *(long*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_lng(dimNum, min, max, step);
		}
		case TYPE_dbl: { 
			double min = *(double*)BUNtail(biter, currBun);
			double max = *(double*)BUNtail(biter, currBun+1);
			double step = *(double*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_dbl(dimNum, min, max, step);
		}
		case TYPE_flt: { 
			float min = *(float*)BUNtail(biter, currBun);
			float max = *(float*)BUNtail(biter, currBun+1);
			float step = *(float*)BUNtail(biter, currBun+2);
			return createAnalyticDimension_flt(dimNum, min, max, step);
		}
		default: 
			return NULL;
	}	
}

static str CMDdimensionCONVERT(gdk_analytic_dimension **dimRes, const gdk_analytic_dimension *dim, int type) {
	int abort_on_error = 1;
	BAT *dimBAT_in, *dimBAT_out, *s = NULL;
	gdk_analytic_dimension *dim_out;

	/* create a BAT and put inside the values of the dimensions */
	/* needed to re-use functions that exist already */
    if(!(dimBAT_in = dimension2BAT(dim)))
        throw(MAL, "batcalc.convert", MAL_MALLOC_FAIL);

	if(!(dimBAT_out = BATconvert(dimBAT_in, s, type, abort_on_error))) {
		char buf[20];
		snprintf(buf, sizeof(buf), "batcalc.%s", ATOMname(type));

		BBPunfix(dimBAT_in->batCacheid);
		return createException(MAL, buf, OPERATION_FAILED);
	}

	BBPunfix(dimBAT_in->batCacheid);

	/*put the converted values from the BAT to the dim */
	if(!(dim_out = BAT2dimension(dimBAT_out, dim->dimNum))) {
		char buf[20];
		snprintf(buf, sizeof(buf), "batcalc.%s", ATOMname(type));
	
		BBPunfix(dimBAT_out->batCacheid);
		return createException(MAL, buf, "Type not handled");
	}

	*dimRes = dim_out;

	BBPunfix(dimBAT_out->batCacheid);

    return MAL_SUCCEED;
}

/*
str CMDdimensionCONVERT_void(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_void);
}

str CMDdimensionCONVERT_bit(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_bit);
}
*/

str CMDdimensionCONVERT_bte(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_bte);
}

str CMDdimensionCONVERT_sht(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_sht);
}

str CMDdimensionCONVERT_int(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_int);
}

str CMDdimensionCONVERT_wrd(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_wrd);
}

str CMDdimensionCONVERT_lng(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_lng);
}

str CMDdimensionCONVERT_flt(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_flt);
}

str CMDdimensionCONVERT_dbl(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_dbl);
}

str CMDdimensionCONVERT_oid(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_oid);
}

/*
str CMDdimensionCONVERT_str(ptr *dimRes, ptr *dimsRes, const ptr *dim, const ptr *dims) {
	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*dims) ;
	*dimsRes = array;
 
	return CMDdimensionCONVERT((gdk_analytic_dimension**)dimRes, (gdk_analytic_dimension*)*dim, TYPE_str);
}*/


str CMDdimensionMULsignal(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci) {
	ptr *dimConverted = getArgReference_ptr(stk, pci, 0);
	ptr *array_out = getArgReference_ptr(stk, pci, 1);

	void *valPtr = getArgReference(stk, pci, 2);
	int valType = getArgType(mb, pci, 2);

	ptr *dimOriginal = getArgReference_ptr(stk, pci, 3);
	ptr *array_in = getArgReference_ptr(stk, pci, 4);
	
	int *resType = getArgReference_int(stk, pci, 5);

	gdk_analytic_dimension *dim_out = NULL;
	gdk_analytic_dimension *dim_in = (gdk_analytic_dimension*)*dimOriginal;
	int dimType = dim_in->type;

	//the array is not affected by the convertion
	//I just need to pass it along
	gdk_array *array = arrayCopy((gdk_array*)*array_in);
	*array_out = array;

    (void) cntxt;

	switch(valType) {
		case TYPE_bte: {
			bte val = *(bte*)valPtr;
			
			switch(dimType) {
				case TYPE_bte: {
					bte min = *(bte*)dim_in->min;
					bte max = *(bte*)dim_in->max;
					bte step = *(bte*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_sht: {
					short min = *(short*)dim_in->min;
					short max = *(short*)dim_in->max;
					short step = *(short*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_int: {
					int min = *(int*)dim_in->min;
					int max = *(int*)dim_in->max;
					int step = *(int*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_wrd: {
					wrd min = *(wrd*)dim_in->min;
					wrd max = *(wrd*)dim_in->max;
					wrd step = *(wrd*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_oid: {
					oid min = *(oid*)dim_in->min;
					oid max = *(oid*)dim_in->max;
					oid step = *(oid*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_dbl: {
					double min = *(double*)dim_in->min;
					double max = *(double*)dim_in->max;
					double step = *(double*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_lng: {
					long min = *(long*)dim_in->min;
					long max = *(long*)dim_in->max;
					long step = *(long*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_flt: {
					float min = *(float*)dim_in->min;
					float max = *(float*)dim_in->max;
					float step = *(float*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				default:
					return createException(MAL, "calc.*", "Dimension type not handles");
			}
		} break;
		case TYPE_int: {
			int val = *(int*)valPtr;
			
			switch(dimType) {
				case TYPE_bte: {
					bte min = *(bte*)dim_in->min;
					bte max = *(bte*)dim_in->max;
					bte step = *(bte*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_sht: {
					short min = *(short*)dim_in->min;
					short max = *(short*)dim_in->max;
					short step = *(short*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_int: {
					int min = *(int*)dim_in->min;
					int max = *(int*)dim_in->max;
					int step = *(int*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_wrd: {
					wrd min = *(wrd*)dim_in->min;
					wrd max = *(wrd*)dim_in->max;
					wrd step = *(wrd*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_oid: {
					oid min = *(oid*)dim_in->min;
					oid max = *(oid*)dim_in->max;
					oid step = *(oid*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_dbl: {
					double min = *(double*)dim_in->min;
					double max = *(double*)dim_in->max;
					double step = *(double*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_lng: {
					long min = *(long*)dim_in->min;
					long max = *(long*)dim_in->max;
					long step = *(long*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_flt: {
					float min = *(float*)dim_in->min;
					float max = *(float*)dim_in->max;
					float step = *(float*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				default:
					return createException(MAL, "calc.*", "Dimension type not handles");
			}

		} break;
		case TYPE_wrd: {
			wrd val = *(wrd*)valPtr;
			
			switch(dimType) {
				case TYPE_bte: {
					bte min = *(bte*)dim_in->min;
					bte max = *(bte*)dim_in->max;
					bte step = *(bte*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_sht: {
					short min = *(short*)dim_in->min;
					short max = *(short*)dim_in->max;
					short step = *(short*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_int: {
					int min = *(int*)dim_in->min;
					int max = *(int*)dim_in->max;
					int step = *(int*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_wrd: {
					wrd min = *(wrd*)dim_in->min;
					wrd max = *(wrd*)dim_in->max;
					wrd step = *(wrd*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_oid: {
					oid min = *(oid*)dim_in->min;
					oid max = *(oid*)dim_in->max;
					oid step = *(oid*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_dbl: {
					double min = *(double*)dim_in->min;
					double max = *(double*)dim_in->max;
					double step = *(double*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_lng: {
					long min = *(long*)dim_in->min;
					long max = *(long*)dim_in->max;
					long step = *(long*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_flt: {
					float min = *(float*)dim_in->min;
					float max = *(float*)dim_in->max;
					float step = *(float*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				default:
					return createException(MAL, "calc.*", "Dimension type not handles");
			}

		} break;
		case TYPE_oid: {
			oid val = *(oid*)valPtr;
			
			switch(dimType) {
				case TYPE_bte: {
					bte min = *(bte*)dim_in->min;
					bte max = *(bte*)dim_in->max;
					bte step = *(bte*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_sht: {
					short min = *(short*)dim_in->min;
					short max = *(short*)dim_in->max;
					short step = *(short*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_int: {
					int min = *(int*)dim_in->min;
					int max = *(int*)dim_in->max;
					int step = *(int*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_wrd: {
					wrd min = *(wrd*)dim_in->min;
					wrd max = *(wrd*)dim_in->max;
					wrd step = *(wrd*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_oid: {
					oid min = *(oid*)dim_in->min;
					oid max = *(oid*)dim_in->max;
					oid step = *(oid*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_dbl: {
					double min = *(double*)dim_in->min;
					double max = *(double*)dim_in->max;
					double step = *(double*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_lng: {
					long min = *(long*)dim_in->min;
					long max = *(long*)dim_in->max;
					long step = *(long*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_flt: {
					float min = *(float*)dim_in->min;
					float max = *(float*)dim_in->max;
					float step = *(float*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				default:
					return createException(MAL, "calc.*", "Dimension type not handles");
			}

		} break;
		case TYPE_dbl: {
			double val = *(double*)valPtr;
			
			switch(dimType) {
				case TYPE_bte: {
					bte min = *(bte*)dim_in->min;
					bte max = *(bte*)dim_in->max;
					bte step = *(bte*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_sht: {
					short min = *(short*)dim_in->min;
					short max = *(short*)dim_in->max;
					short step = *(short*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_int: {
					int min = *(int*)dim_in->min;
					int max = *(int*)dim_in->max;
					int step = *(int*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_wrd: {
					wrd min = *(wrd*)dim_in->min;
					wrd max = *(wrd*)dim_in->max;
					wrd step = *(wrd*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_oid: {
					oid min = *(oid*)dim_in->min;
					oid max = *(oid*)dim_in->max;
					oid step = *(oid*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_dbl: {
					double min = *(double*)dim_in->min;
					double max = *(double*)dim_in->max;
					double step = *(double*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_lng: {
					long min = *(long*)dim_in->min;
					long max = *(long*)dim_in->max;
					long step = *(long*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_flt: {
					float min = *(float*)dim_in->min;
					float max = *(float*)dim_in->max;
					float step = *(float*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				default:
					return createException(MAL, "calc.*", "Dimension type not handles");
			}

		} break;
		case TYPE_lng: {
			long val = *(long*)valPtr;
			
			switch(dimType) {
				case TYPE_bte: {
					bte min = *(bte*)dim_in->min;
					bte max = *(bte*)dim_in->max;
					bte step = *(bte*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_sht: {
					short min = *(short*)dim_in->min;
					short max = *(short*)dim_in->max;
					short step = *(short*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_int: {
					int min = *(int*)dim_in->min;
					int max = *(int*)dim_in->max;
					int step = *(int*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_wrd: {
					wrd min = *(wrd*)dim_in->min;
					wrd max = *(wrd*)dim_in->max;
					wrd step = *(wrd*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_oid: {
					oid min = *(oid*)dim_in->min;
					oid max = *(oid*)dim_in->max;
					oid step = *(oid*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_dbl: {
					double min = *(double*)dim_in->min;
					double max = *(double*)dim_in->max;
					double step = *(double*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_lng: {
					long min = *(long*)dim_in->min;
					long max = *(long*)dim_in->max;
					long step = *(long*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_flt: {
					float min = *(float*)dim_in->min;
					float max = *(float*)dim_in->max;
					float step = *(float*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				default:
					return createException(MAL, "calc.*", "Dimension type not handles");
			}

		} break;
		case TYPE_flt: {
			float val = *(float*)valPtr;
			
			switch(dimType) {
				case TYPE_bte: {
					bte min = *(bte*)dim_in->min;
					bte max = *(bte*)dim_in->max;
					bte step = *(bte*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_sht: {
					short min = *(short*)dim_in->min;
					short max = *(short*)dim_in->max;
					short step = *(short*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val); 
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_int: {
					int min = *(int*)dim_in->min;
					int max = *(int*)dim_in->max;
					int step = *(int*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_wrd: {
					wrd min = *(wrd*)dim_in->min;
					wrd max = *(wrd*)dim_in->max;
					wrd step = *(wrd*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_oid: {
					oid min = *(oid*)dim_in->min;
					oid max = *(oid*)dim_in->max;
					oid step = *(oid*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_dbl: {
					double min = *(double*)dim_in->min;
					double max = *(double*)dim_in->max;
					double step = *(double*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_lng: {
					long min = *(long*)dim_in->min;
					long max = *(long*)dim_in->max;
					long step = *(long*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				case TYPE_flt: {
					float min = *(float*)dim_in->min;
					float max = *(float*)dim_in->max;
					float step = *(float*)dim_in->step;

					switch(*resType){
						case TYPE_bte:
							dim_out =createAnalyticDimension_bte(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_sht:
							dim_out =createAnalyticDimension_sht(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_int:
							dim_out =createAnalyticDimension_int(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_wrd:
							dim_out =createAnalyticDimension_wrd(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_oid:
							dim_out =createAnalyticDimension_oid(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_dbl:
							dim_out =createAnalyticDimension_dbl(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_lng:
							dim_out =createAnalyticDimension_lng(dim_in->dimNum, min*val, max*val, step*val);
							break;
						case TYPE_flt:
							dim_out =createAnalyticDimension_flt(dim_in->dimNum, min*val, max*val, step*val);
							break;
						default:
							return createException(MAL, "calc.*", "Return type not handles");
					}
				} break;
				default:
					return createException(MAL, "calc.*", "Dimension type not handles");
			}

		} break;
		default:
			return createException(MAL, "calc.*", "value type not handles");
	}

	*dimConverted = dim_out;
	return MAL_SUCCEED;
}

#define checkEqual(TPE1, dimLeft, TPE2, dimRight) \
do { \
	BUN i=0; \
	TPE1 valLeft; \
	TPE1 maxLeft = *(TPE1*)dimLeft->max; \
	TPE1 stepLeft = *(TPE1)dimLeft->step; \
	TPE2 valRight; \
	TPE2 maxRight = *(TPE2*)dimRight->max; \
	TPE2 stepRight = *(TPE2)dimRight->step; \
\
	for(valLeft = *(TPE1*)dimLeft->min ; valLeft<=maxLeft; valLeft += stepLeft)
} while(0);

str CMDdimensionEQ(ptr* dimsRes, bat* batRes, const ptr* dim1, const ptr* dims1, const ptr* dim2, const ptr* dims2) {
	gdk_analytic_dimension *dimLeft = (gdk_analytic_dimension*)*dim1;
	gdk_analytic_dimension *dimRight = (gdk_analytic_dimension*)*dim2;

	BUN i = 0;

	/* create a BAT of size equal to dimLeft->elsNum * dimRight->elsNum
 	* There is no need to add more detailed info no matter how many the 
 	* dimensions because the same is true for each one of the values of
 	* the other dimensions */
	BUN elsNum = (dimLeft->elsNum)*(dimRight->elsNum);
	BAT *resBAT = BATnew(TYPE_void, TYPE_bit, elsNum, TRANSIENT);
	bit *vals;
	if(!resBAT)
		return createException(MAL, "calc.==", MAL_MALLOC_FAIL);
	vals = (bit*)Tloc(resBAT, BUNfirst(resBAT));

	for(i=0; i<elsNum; i++) {
		
	}
	
	(void)*dimsRes;
	(void)*batRes;
	(void)*dimLeft;
	(void)*dims1;
	(void)*dimRight;
	(void)*dims2;
/*
    if (VARcalceq(&stk->stk[getArg(pci, 0)], &stk->stk[getArg(pci, 1)], &stk->stk[getArg(pci, 2)]) != GDK_SUCCEED)
        return mythrow(MAL, "calc.==", OPERATION_FAILED);
*/
    return MAL_SUCCEED;
}

