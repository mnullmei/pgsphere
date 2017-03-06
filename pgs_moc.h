#ifndef __PGS_MOC_H__
#define __PGS_MOC_H__

#include <postgres.h>
#include <fmgr.h>
#include <catalog/pg_type.h>

#include "point.h" /* SPoint */
#include "pgs_healpix.h"
#include "pgs_process_moc.h"


/*
 * MOC data type(s)
 * ...
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */

/* das hier ist Mist: typedef moc_interval moc_leaf_page[MOC_PAGE_SIZE]; */


/*	move the below to ^^start of pgs_moc.h ;-)

Layout of pages:
tree pages:
* a single int32 'level_end' value == (TOAST) offset value of end of the _next_
  deeper level
  -> was: int32 root_size [[number of moc_tree_entry items in root page]]
* array of moc_tree_entry

-> this means that the offset of the first interval is actually always at the
   same place, it is the 'offset' value of the first moc_tree_entry of the
   "root page", this makes a special entry inside moc_header redundant.

	computationally, the offset of first interval is: ...

it can bee easily computed if a page (tree or leaf) does not start on
a PG_TOAST_PAGE_FRAGMENT boundary by taking the modulus of the start offset by
exactly PG_TOAST_PAGE_FRAGMENT...
	
*/

/* function prototypes for the MOC support functions */

Datum smoc_in(PG_FUNCTION_ARGS);
Datum smoc_out(PG_FUNCTION_ARGS);
Datum moc_debug(PG_FUNCTION_ARGS);
Datum set_smoc_output_type(PG_FUNCTION_ARGS);

/* parsing subroutines */
hpint64 readNumber(const char*, int*);
char readChar(const char*, int*);

#endif
