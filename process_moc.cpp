#include <cstddef>
#include <exception>
#include <set>
#include  <algorithm>

#include "pgs_process_moc.h"

#define PGS_TRY try	{
#define PGS_CATCH 	}						\
	catch (std::exception & e)				\
	{										\
		delete m;							\
		error_out(e.what(), 0);				\
	}										\
	catch (...)								\
	{										\
		delete m;							\
		error_out("unknown exception", 0);	\
	}


bool
operator<(const moc_interval & x, const moc_interval & y)
{
	return x.first < y.first;
}

typedef std::set<moc_interval> moc_map;

struct moc_input
{
	moc_map input_map;
};

void*
create_moc_context(pgs_error_handler error_out)
{
	moc_input* m = 0;
	PGS_TRY
		m = new moc_input;
	PGS_CATCH
	return static_cast<void*>(m);
};

void
release_moc_context(void* moc_context, pgs_error_handler error_out)
{
	moc_input* m = static_cast<moc_input*>(moc_context);
	try
	{
		delete m;
	}
	catch (std::exception & e)
	{
		error_out(e.what(), 0);
	}
	catch (...)
	{
		error_out("unknown exception", 0);
	}
}

int
add_to_moc(void* moc_context, long order, hpint64 first, hpint64 last,
												pgs_error_handler error_out)
{
	moc_input* m = static_cast<moc_input*>(moc_context);
	PGS_TRY
		
	PGS_CATCH
	return 0;
};

// get_moc_size() prepares creation of MOC

int
get_moc_size(void* moc_context, pgs_error_handler error_out)
{
	moc_input* m = static_cast<moc_input*>(moc_context);
	std::size_t moc_size = MOC_HEADER_SIZE;
	PGS_TRY
		
		moc_size = std::max(MIN_MOC_SIZE, moc_size);
	PGS_CATCH
	return moc_size;
};

// create_moc_release_context()
// moc_context:	-- must be have been prepared by get_moc_size()
// moc:			-- must be allocated with a size returned by get_moc_size()
//

int
create_moc_release_context(void* moc_context, Smoc* moc,
												pgs_error_handler error_out)
{
	moc_input* m = static_cast<moc_input*>(moc_context);
	int ret = 1;
	PGS_TRY
		hpint64	area = 0; /* number of covered Healpix cells */
		int root_end = MOC_HEADER_SIZE;
		int data_end = MOC_HEADER_SIZE;
		
		moc->version	= 0;
		moc->order		= 0 /* ... */;
		moc->depth		= 0 /* ... */;
		moc->first		= 0 /* ... */;	/* first Healpix index in set */
		moc->last		= 0 /* ... */;	/* 1 + (last Healpix index in set) */
		moc->area		= 0 /* ... */;
		moc->root_end	= root_end; /* 1 + (enf of root node) */
		moc->data_end	= data_end; /* 1 + (offset of last interval) */
		
	PGS_CATCH
	release_moc_context(moc_context, error_out);
	return ret;
};
