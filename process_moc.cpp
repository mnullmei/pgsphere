#include <cstddef>
#include <cstring>
#include <exception>
#include <set>
#include <algorithm>
#include <ostream>
#include <iostream>
#include <string>
#include <sstream>

#include "pgs_process_moc.h"

#define PGS_TRY try	{ do {
#define PGS_CATCH 	} while (0); }			\
	catch (std::exception & e)				\
	{										\
		delete p;							\
		error_out(e.what(), 0);				\
	}										\
	catch (...)								\
	{										\
		delete p;							\
		error_out("unknown exception", 0);	\
	}

static void
healpix_convert(hpint64 & idx, int32 from_order)
{
	idx <<= (29 - from_order) * 2;
}

moc_interval
make_interval(hpint64 first, hpint64 last)
{
	moc_interval x;
	x.first = first;
	x.second = last;
	return x;
}

bool
operator<(const moc_interval & x, const moc_interval & y)
{
	return x.first < y.first;
}

std::ostream &
operator<<(std::ostream & os, const moc_interval & x)
{
	os << "[" << x.first << ", " << x.second << ")";
	return os;
}

typedef std::set<moc_interval> moc_map;
typedef moc_map::iterator map_iterator;

struct moc_input
{
	moc_map input_map;
	std::size_t moc_size;
	std::string s;
	moc_input() : moc_size(0) {}
};

void*
create_moc_context(pgs_error_handler error_out)
{
	moc_input* p = 0;
	PGS_TRY
		p = new moc_input;
	PGS_CATCH
	return static_cast<void*>(p);
};

void
release_moc_context(void* moc_context, pgs_error_handler error_out)
{
	moc_input* p = static_cast<moc_input*>(moc_context);
	try
	{
		delete p;
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
	moc_input* p = static_cast<moc_input*>(moc_context);
	PGS_TRY
		moc_input & m = *p;
		healpix_convert(first, order); // convert to order 29
		healpix_convert(last, order);
		moc_interval input = make_interval(first, last);
		map_iterator lower = m.input_map.lower_bound(input);
		if (lower != m.input_map.begin())
		{
			map_iterator before = lower;
			--before;

//			input.first = lower->first;

//m.input_map.insert(make_interval(input.first, 100 * lower->first));

		}
		// interval past the current one, if any
		map_iterator upper = m.input_map.upper_bound(make_interval(last, 0));
// 		if (lower == upper)
// 		{
// 			// This path would be superflous with C++11's erase(), as it returns
// 			// the correct hint for the insert() of the general case down below.
// 			m.input_map.insert(lower, input);
// 			break;
// 		}
//		m.input_map.erase(lower, upper);
		m.input_map.insert(input);

m.input_map.insert(make_interval(10000 + input.first, input.second));

	PGS_CATCH
	return 0;
};

// get_moc_size() prepares creation of MOC

int
get_moc_size(void* moc_context, pgs_error_handler error_out)
{
	moc_input* p = static_cast<moc_input*>(moc_context);
	std::size_t moc_size = MOC_HEADER_SIZE;
	PGS_TRY
		moc_input & m = *p;
//prelim: do a string...
		std::ostringstream oss;
		oss << (m.input_map.size() ? "{" : "{ ");
		for (map_iterator i = m.input_map.begin(); i != m.input_map.end(); ++i)
			oss << *i << " ";
		m.s = oss.str();
		*m.s.rbegin() = '}';
		moc_size = MOC_HEADER_SIZE + m.s.size() + 1;
		
		moc_size = std::max(MIN_MOC_SIZE, moc_size);
		m.moc_size = moc_size;
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
	moc_input* p = static_cast<moc_input*>(moc_context);
	int ret = 1;
	PGS_TRY
		moc_input & m = *p;

		hpint64	area = 0; /* number of covered Healpix cells */
		int root_end = MOC_HEADER_SIZE;

	
		moc->version	= 0;
		moc->order		= 0 /* ... */;
		moc->depth		= 0 /* ... */;
		moc->first		= 0 /* ... */;	/* first Healpix index in set */
		moc->last		= 0 /* ... */;	/* 1 + (last Healpix index in set) */
		moc->area		= area;
		moc->root_end	= root_end;		// 1 + (enf of root node)
		moc->data_end	= m.moc_size;	// 1 + (offset of last interval)
		
		memmove(&(moc->data), m.s.c_str(), m.moc_size - MOC_HEADER_SIZE);
	PGS_CATCH
	release_moc_context(moc_context, error_out);
	return ret;
};
