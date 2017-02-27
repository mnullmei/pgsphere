#include <cstddef>
#include <cstring>
#include <exception>
#include <map>
#include <vector>
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

using std::size_t;

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

static
char* data_as_char(Smoc* moc, size_t offset = 0)
{
	return offset + reinterpret_cast<char*>(&((moc->data)[0]));
}

template<class X, class Y>
static
X* data_as(Y* y)
{
	return reinterpret_cast<X*>(y);
}

static
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


typedef std::map<hpint64, hpint64>	moc_map;
typedef moc_map::iterator			map_iterator;
typedef moc_map::value_type			moc_map_entry;
std::ostream &
operator<<(std::ostream & os, const moc_map_entry & x)
{
	os << "[" << x.first << ", " << x.second << ")";
	return os;
}

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

struct moc_tree_layout
{
	size_t entries;		// # all nodes
	size_t page_rest;	// # bytes
	size_t rest_nodes;	// # nodes
	size_t rest_level;	// # nodes
	size_t full_pages;	// # pages 
	size_t last_page;	// # nodes
	size_t level_end;	// index of next entity below this level
	moc_tree_layout(): entries(0), page_rest(0), rest_nodes(0), rest_level(0),
									full_pages(0), last_page(0), level_end(0) {}
	moc_tree_layout(size_t len): entries(len), page_rest(0), rest_nodes(0),
									rest_level(0), full_pages(0), last_page(0),
									level_end(0) {}
};

typedef std::vector<moc_tree_layout> layout_vec;

struct moc_input
{
	moc_map		input_map;
	size_t		options_bytes;
	size_t		options_size;
	size_t		tree_begin;
	layout_vec	layout;

	std::string s;
	char x[99999];
	moc_input() : options_bytes(0), options_size(0), tree_begin(0)
	{
		layout.reserve(5);
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

void 
layout_level(size_t & moc_size, moc_tree_layout & q, size_t entry_size)
{
	size_t page_len = PG_TOAST_PAGE_FRAGMENT / entry_size;
	q.page_rest = page_len - moc_size % page_len;
	q.rest_nodes = q.page_rest / entry_size;
	q.rest_level = q.entries >= q.rest_nodes ? q.entries - q.rest_nodes : 0;
	q.full_pages = q.rest_level / page_len;
	q.last_page = q.rest_level % entry_size;
	if (q.full_pages || q.last_page)
		moc_size += q.page_rest + PG_TOAST_PAGE_FRAGMENT * q.full_pages
								+ entry_size * q.last_page;
	else
		moc_size += q.entries * entry_size;
	q.level_end = moc_size;
}


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

		// a refactored, C++-interal version of add_to_moc() should start here:
		map_iterator lower = m.input_map.lower_bound(first);
		map_iterator upper = m.input_map.upper_bound(last);

		if (lower != m.input_map.begin())
		{
			map_iterator before = lower;
			--before;
			if (before->second >= first)
			{
				if (before->second >= last)
					break; // [first, last) \subset [before]
				lower = before;
				first = lower->first;
			}
		}
		if (upper != m.input_map.begin())
		{
			map_iterator after = upper;
			--after;
			if (after->second > last)
				last = after->second;
		}
		// Skip erase if it would do nothing in order to be able to use
		// an input hint for set::insert().
		// This path would be superflous with C++11's erase(), as that returns
		// the correct hint for the insert() of the general case down below.
		// The input hint lower == upper always refers the interval completely
		// past the one to insert, or to input_map.end()
		moc_map_entry input(first, last);
		if (lower == upper)
		{
			m.input_map.insert(lower, input);
			break;
		}
		if (lower->first == first)
		{
			lower->second = last;
			m.input_map.erase(++lower, upper);
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
	size_t moc_size = MOC_HEADER_SIZE;
	PGS_TRY
		moc_input & m = *p;

// put the debug string squarely into the moc options header.
		m.s.clear();

		m.dump();
		m.options_bytes = m.s.size() + 1;
		m.options_size = align_round(m.options_bytes, MOC_INDEX_ALIGN);
		moc_size += m.options_size;

		m.tree_begin = moc_size;
		// calculate number of nodes of the B+-tree layout
		size_t len = m.input_map.size();
		m.layout.push_back(len);
		len = 1 + align_round(len, MOC_LEAF_PAGE_LEN);
		bool not_root;
		do
		{
			not_root = len > MOC_TREE_PAGE_LEN;
			m.layout.push_back(len);
			len = 1 + align_round(len, MOC_TREE_PAGE_LEN);
		}
		while (not_root);
		// layout: end of B+-tree level-end section
		size_t depth = m.layout.size() - 1;
		moc_size += depth * MOC_INDEX_ALIGN;
		// layout: B+-tree layout
		for (unsigned k = depth; k >= 1; --k)
			layout_level(moc_size, m.layout[k], MOC_TREE_ENTRY_SIZE);
		// layout: intervals
		moc_size = align_round(moc_size, HP64_SIZE);
		m.layout[1].level_end = moc_size; // fix up alignment of intervals
		layout_level(moc_size, m.layout[0], MOC_INTERVAL_SIZE);

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

		moc->version = 0;
		char* data = data_as_char(moc);

		// put the debug string squarely into the moc options header.
		memmove(data, m.s.c_str(), m.options_bytes);

		hpint64	area = 0; /* number of covered Healpix cells */
area = 9223372036854775807; /* 2^63 - 1 */

		moc->tree_begin	= m.tree_begin;	// start of level-end section

		char* moc_data = data - MOC_HEADER_SIZE;

		// fill out level-end section
		int32* level_ends = data_as<int32>(moc_data + m.tree_begin);
		uint8 depth = m.layout.size() - 1;
		moc->depth	= depth /* ... */;
		for (unsigned k = depth; k >= 1; --k)
			*(level_ends + depth - k) = m.layout[k].level_end;

		// process the interval pages
		map_iterator i	= m.input_map.begin();
		char* intervals	= moc_data + 


		// fill out tree levels



		moc->order		= 0 /* ... */;
		moc->first		= 0 /* ... */;	/* first Healpix index in set */
		moc->last		= 0 /* ... */;	/* 1 + (last Healpix index in set) */
		moc->area		= area;
		
		
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

		// There must be an option to output pure intervals as well!

		// moc output fiddling:


		if (moc->tree_begin != MOC_HEADER_SIZE)
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
