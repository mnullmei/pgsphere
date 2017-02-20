#include <exception>
#include <map>

#include "pgs_process_moc.h"

#define PGS_TRY 1
#define PGS_CATCH 2

bool
operator<(const moc_interval & x, const moc_interval & y) const
{
	return x.first < y.first;
}

typedef std::map<moc_interval> moc_map;

struct moc_input
{
	moc_map input_map;
};

void*
create_moc_context(pgs_error_handler error_out)
{
	moc_input* m = 0;
	try
	{
		m = new moc_input;
	}
	catch(std::exception & e)
	{
		delete m;
		error_out(e.what(), 0);
	}
	catch(...)
	{
		delete m;
		error_out("unknown exception", 0);
	}
	return static_cast<void*>(m);
};

void
release_moc_context(void* moc_context, pgs_error_handler error_out)
{
	moc_input* m = static_cast<moc_input*>(moc_context);
	
		delete m;
		m = 0;
	
}

int
add_to_moc(void* moc_context, long order, hpint64 first, hpint64 last,
												pgs_error_handler error_out);
{
	return 0;
};

int
get_moc_size(void* moc_context, pgs_error_handler error_out)
{
	return 0;
};

int
create_moc_release_context(void* moc_context, Smoc* moc,
												pgs_error_handler error_out);
{
	moc->version	= 0;
	moc->order		= 0 /* ... */;
	moc->first		= 0 /* ... */;	/* first Healpix index in set */
	moc->last		= 0 /* ... */;	/* 1 + (last Healpix index in set) */
	moc->area		= 0 /* ... */;
	moc->end		= 0 /* ... */;	/* 1 + (offset of last interval) */


	release_moc_context(moc_context, error_out);
	return 0;
};

