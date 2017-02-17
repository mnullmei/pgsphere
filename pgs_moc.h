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

typedef struct
{
	hpint64 first;
	hpint64 second;
} moc_interval;

#define MOC_PAGE_SIZE (1010 / sizeof(moc_interval))
#define HP64_SIZE (sizeof(hpint64))

typedef struct
{
	int32	offset;
	char	start[HP64_SIZE];
} moc_tree_entry;

typedef moc_interval moc_leaf_page[MOC_PAGE_SIZE];

typedef struct
{
	char	vl_len_[4];
	int32	
	
	
				
} moc_header;

typedef struct
{
	union
	{
		moc_header	header;
		hpint64		align_dummy[(1 +
						sizeof(moc_header) / sizeof(hpint64)) * sizeof(hpint64)]
	};
	char		data[FLEXIBLE_ARRAY_MEMBER];
} Smoc;

/* function prototypes for the MOC support functions */


#endif
