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

// PGS_TRY / PGS_CATCH: use an additional 'do {} while (0);' to allow for
// 'break;' as an alternative to 'return;'

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

// Throwing expections from destructors is not strictly forbidden, it is just
// discouraged in the strongest possible way.
template <class C>
void
release_context(void* context, pgs_error_handler error_out)
{
	C* p = static_cast<C*>(context);
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

char* data_as_char(Smoc* moc, size_t offset = 0)
{
	return offset + reinterpret_cast<char*>(&((moc->data)[0]));
}

size_t align_round(size_t offset, size_t alignment)
{
	return (1 + offset / alignment) * alignment;
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

struct moc_output
{
	std::string s;
};

struct moc_input
{
	moc_map input_map;
	std::size_t options_bytes;
	std::size_t options_size;
	std::size_t root_begin;
	std::string s;
	char x[99999];
	moc_input() : options_bytes(0), options_size(0), root_begin(0)
	{
		x[0] = '\0';
	}
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
create_moc_in_context(pgs_error_handler error_out)
{
	moc_input* p = 0;
	PGS_TRY
		p = new moc_input;
	PGS_CATCH
	return static_cast<void*>(p);
};

void
release_moc_in_context(void* moc_in_context, pgs_error_handler error_out)
{
	release_context<moc_input>(moc_in_context, error_out);
}

char*
add_to_moc(void* moc_in_context, long order, hpint64 first, hpint64 last,
												pgs_error_handler error_out)
{
	moc_input* p = static_cast<moc_input*>(moc_in_context);
	PGS_TRY
		moc_input & m = *p;

		healpix_convert(first, order); // convert to order 29
		healpix_convert(last, order);
		moc_interval input = make_interval(first, last);

		// a refactored, C++-interal version of add_to_moc() should start here:
		map_iterator lower = m.input_map.lower_bound(input);
		map_iterator upper = m.input_map.upper_bound(make_interval(last, 0));

		if (lower != m.input_map.begin())
		{
			map_iterator before = lower;
			--before;
			if (before->second >= input.first)
			{
				if (before->second >= input.second)
					break; // input \subset [before]
				lower = before;
				input.first = lower->first;
			}
		}
		if (upper != m.input_map.begin())
		{
			map_iterator after = upper;
			--after;
			if (after->second > input.second)
				input.second = after->second;
		}
		// Skip erase if it would do nothing in order to be able to use
		// an input hint for set::insert().
		// This path would be superflous with C++11's erase(), as that returns
		// the correct hint for the insert() of the general case down below.
		// The input hint lower == upper always refers the interval completely
		// past the one to insert, or to input_map.end()
		if (lower == upper)
		{
			m.input_map.insert(lower, input);
			break;
		}
		m.input_map.erase(lower, upper);
		m.input_map.insert(input);
	PGS_CATCH
	return p->x;
};

// get_moc_size() prepares creation of MOC

int
get_moc_size(void* moc_in_context, pgs_error_handler error_out)
{
	moc_input* p = static_cast<moc_input*>(moc_in_context);
	std::size_t moc_size = MOC_HEADER_SIZE;
	PGS_TRY
		moc_input & m = *p;

// put the debug string squarely into the moc options header.
		m.s.clear();


		m.dump();
		m.options_bytes = m.s.size() + 1;
		m.options_size = align_round(m.options_bytes, MOC_DATA_ALIGN);
		moc_size += m.options_size;
		m.root_begin = moc_size;

		moc_size = std::max(MIN_MOC_SIZE, moc_size);
	PGS_CATCH
	return moc_size;
};

// create_moc_release_context()
// moc_in_context:	-- must be have been prepared by get_moc_size()
// moc:			-- must be allocated with a size returned by get_moc_size()
//

int
create_moc_release_context(void* moc_in_context, Smoc* moc,
													pgs_error_handler error_out)
{
	moc_input* p = static_cast<moc_input*>(moc_in_context);
	int ret = 1;
	PGS_TRY
		moc_input & m = *p;

		hpint64	area = 0; /* number of covered Healpix cells */


		moc->version	= 0;
		moc->order		= 0 /* ... */;
		moc->depth		= 0 /* ... */;
		moc->first		= 0 /* ... */;	/* first Healpix index in set */
		moc->last		= 0 /* ... */;	/* 1 + (last Healpix index in set) */
		moc->area		= area;
		moc->root_begin	= m.root_begin;	// start of root node
		moc->root_end	= m.root_begin;	// 1 + (end of root node)
		
		char* data = data_as_char(moc);
		
		// put the debug string squarely into the moc options header.
		memmove(data, m.s.c_str(), m.options_bytes);
//		memset(data + m.options_size, 0, m.options_size - m.options_bytes);
	PGS_CATCH
	release_moc_in_context(moc_in_context, error_out);
	return ret;
};

moc_out_data
create_moc_out_context(Smoc* moc, pgs_error_handler error_out)
{
	moc_output* p = 0;
	size_t out_size = 0;
	PGS_TRY
		p = new moc_output;
		moc_output & m = *p;

		// moc output fiddling:


		if (moc->root_begin != MOC_HEADER_SIZE)
		{
			/* assume a raw C string within the MOC options for now */
			m.s.append(data_as_char(moc));
		}

		out_size = m.s.length() + 1;
	PGS_CATCH
	moc_out_data ret;
	ret.context = static_cast<void*>(p);
	ret.out_size = out_size;
	return ret;
}

void 
release_moc_out_context(moc_out_data out_context, pgs_error_handler error_out)
{
	release_context<moc_output>(out_context.context, error_out);
}

void
print_moc_release_context(moc_out_data out_context, char* buffer,
													pgs_error_handler error_out)
{
	moc_output* p = static_cast<moc_output*>(out_context.context);
	PGS_TRY
		moc_output & m = *p;
		memmove(buffer, m.s.c_str(), out_context.out_size);
	PGS_CATCH
	release_moc_out_context(out_context, error_out);
}
