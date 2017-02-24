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

template<class X>
std::string to_string(const X & x)
{
	std::ostringstream oss;
	oss << x;
	return oss.str();
}

struct moc_input
{
	moc_map input_map;
	std::size_t moc_size;
	std::string s;
	char x[99999];
	moc_input() : moc_size(0) {}
	void dump()
	{
		std::ostringstream oss;
		oss << (input_map.size() ? "{" : "{ ");
		for (map_iterator i = input_map.begin(); i != input_map.end(); ++i)
			oss << *i << " ";
		s.append(oss.str());
		*s.rbegin() = '}';
	}
	void lndump(const std::string & msg)
	{
		s.append("\n");
		s.append(msg + ":\n");
		dump();
	}
	void addln(const std::string & msg)
	{
		s.append("\n");
		s.append(msg);
	}
	std::string to_string(map_iterator i)
	{
		if (i == input_map.end())
			return "[END]";
		return std::string(i == input_map.begin() ? "[BEGIN]" : "")
				+ ::to_string(*i);
	}
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

char*
add_to_moc(void* moc_context, long order, hpint64 first, hpint64 last,
												pgs_error_handler error_out)
{
	moc_input* p = static_cast<moc_input*>(moc_context);
	PGS_TRY
		moc_input & m = *p;
m.s.clear();
m.lndump("entry");

		healpix_convert(first, order); // convert to order 29
		healpix_convert(last, order);
		moc_interval input = make_interval(first, last);
		map_iterator lower = m.input_map.lower_bound(input);
		map_iterator upper = m.input_map.upper_bound(make_interval(last, 0));

m.addln(std::string("input = ") + to_string(input));
m.addln(to_string("lower = ") + m.to_string(lower));
m.addln(to_string("upper = ") + m.to_string(upper));

		if (lower != m.input_map.begin())
		{
m.lndump("have before");
			map_iterator before = lower;
			--before;
m.addln(std::string("before = ") + m.to_string(before));

			if (before->second >= input.first)
			{
m.addln("connect_front?");
				if (before->second >= input.second)
				{
m.addln("nothing to do here, go away :-) -- input \\subset [before]");
					goto go_away; 
				}

m.lndump("++connect_front:");
				lower = before;
m.addln(to_string("changed: lower = ") + m.to_string(lower));

				input.first = lower->first;
m.addln(std::string("changed: input = ") + to_string(input));

			}
		}

		if (upper != m.input_map.begin())
		{
m.lndump("have after");
			map_iterator after = upper;
			--after;
m.addln(std::string("after = ") + m.to_string(after));

			if (after->second > input.second)
			{
m.addln("++connect_back.");
				input.second = after->second;
m.addln(std::string("changed: input = ") + to_string(input));

			}

		}

		// Skip erase if it would do nothing in order to be able to use
		// an input hint for set::insert().
		// This path would be superflous with C++11's erase(), as it returns
		// the correct hint for the insert() of the general case down below.
		// lower == upper always refers the interval completely past 
		// the one to insert, or to input_map.end()
		if (lower == upper)
		{
m.lndump("lower == upper");
m.addln(std::string("add input: ") + to_string(input));
			m.input_map.insert(lower, input);
			goto go_away; // break;
		}

m.lndump("before erase");
m.addln(to_string("lower = ") + m.to_string(lower));
m.addln(to_string("upper = ") + m.to_string(upper));

		m.input_map.erase(lower, upper);

m.lndump("after erase");

		m.input_map.insert(input);

m.lndump("after insert");

go_away: memmove(m.x, m.s.c_str(), m.s.length() + 1);

	PGS_CATCH
	return p->x;
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
		m.s.clear();
		m.dump();
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
