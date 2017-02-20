#ifndef __PGS_PROCESS_MOC_H__
#define __PGS_PROCESS_MOC_H__

#include <chealpix.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct
{
	char			vl_len_[4];	/* size of PostgreSQL variable-length data */
	uint16			version;	/* version of the 'toasty' MOC data structure */
	uint8			order;		/* actual MOC order */
	uint8			depth;		/* depth of B+-tree */
	hpint64			first;		/* first Healpix index in set */
	hpint64			last;		/* 1 + (last Healpix index in set) */
	hpint64			area;		/* number of covered Healpix cells */
	int32			root_end;	/* end of root node entries */
	int32			data_end;	/* 1 + (offset of last interval) */
	moc_interval	data[1];	/* no need to optimise for empty MOCs */
} Smoc;

/* the redundant data_end allows to skip reading the tree before a "sequential
 * scan" inside a single MOC, thus minising the number of TOAST page reads
 */

#define MOC_HEADER_SIZE (offsetof(Smoc, data))

void*
create_moc_context(void);

void
release_moc_context(void*);

int
add_to_moc(void*, hpint64, hpint64);

int
get_moc_size(void*);

int
create_moc(void*, Smoc*);

#ifdef __cplusplus
}
#endif

#endif
