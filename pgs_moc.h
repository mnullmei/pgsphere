#ifndef __PGS_MOC_H__
#define __PGS_MOC_H__

#include <postgres.h>
#include <fmgr.h>
#include <catalog/pg_type.h>

#include <point.h> /* SPoint */
#include <pgs_healpix.h>

#include <chealpix.h>

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

#define PG_TOAST_PAGE_FRAGMENT 1010
#define HP64_SIZE (sizeof(hpint64))

typedef struct
{
	hpint64 first;
	hpint64 second;
} moc_interval;

typedef struct
{
	int32	offset;				/* counts in units of char */
	char	start[HP64_SIZE];
} moc_tree_entry;

#define MOC_LEAF_PAGE_SIZE (PG_TOAST_PAGE_FRAGMENT / sizeof(moc_interval))
#define MOC_TREE_PAGE_SIZE (PG_TOAST_PAGE_FRAGMENT / sizeof(moc_tree_entry))

/* das hier ist Mist: typedef moc_interval moc_leaf_page[MOC_PAGE_SIZE]; */

typedef struct
{
	char			vl_len_[4];	/* size of PostgreSQL variable-length data */
	uint16			version;	/* version of the 'toasty' MOC data structure */
	uint8			order;		/* actual MOC order */
	uint8			reserved;
	hpint64			first;		/* first Healpix index in set */
	hpint64			start;		/* 1 + (last Healpix index in set) */
	int32			end;		/* 1 + (offset of last interval) */
					/* ^ for "sequential scan" inside this MOC... */
						/* dont't forget to palloc0() the above stuff... */
	moc_interval	data[FLEXIBLE_ARRAY_MEMBER];
} Smoc;

/*	move the below to ^^start of pgs_moc.h ;-)

Layout of pages:
tree pages:
* a single int32 'level_end' value == (TOAST) offset value of end of this level
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
if else case default

/* function prototypes for the MOC support functions */


#endif
