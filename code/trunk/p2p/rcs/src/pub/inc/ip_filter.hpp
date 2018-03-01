#ifndef HEADER_IP_FILTER_HPP
#define HEADER_IP_FILTER_HPP


#include "depend.hpp"
#include "bc_assert.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{
enum AccessFlag
{
	NULLACCESSFLAG = 0,
	BLOCKED =1,
	OUTER = 2, 
	INNER = 3
};

inline bool operator<=(ip_address const& lhs, ip_address const& rhs)
{
	return lhs < rhs || lhs == rhs;
}

template <class Addr>
struct IpRange
{
	Addr first;
	Addr last;
	int access;
};

namespace Detail
{

template<class Addr>
Addr Zero()
{
	Addr zero;
	std::fill(zero.begin(), zero.end(), 0);
	return zero;
}

template<>
inline boost::uint16_t Zero<boost::uint16_t>() { return 0; }

template<class Addr>
Addr PlusOne(Addr const& a)
{
	Addr temp(a);
	for (int i = temp.size() - 1; i >= 0; --i)
	{
		if (temp[i] < (std::numeric_limits<typename Addr::value_type>::max)())
		{
			temp[i] += 1;
			break;
		}
		temp[i] = 0;
	}
	return temp;
}

inline boost::uint16_t PlusOne(boost::uint16_t val) { return val + 1; }

template<class Addr>
Addr MinusOne(Addr const& a)
{
	Addr tmp(a);
	for (int i = tmp.size() - 1; i >= 0; --i)
	{
		if (tmp[i] > 0)
		{
			tmp[i] -= 1;
			break;
		}
		tmp[i] = (std::numeric_limits<typename Addr::value_type>::max)();
	}
	return tmp;
}

inline boost::uint16_t MinusOne(boost::uint16_t val) { return val - 1; }

template<class Addr>
Addr MaxAddr()
{
	Addr tmp;
	std::fill(tmp.begin(), tmp.end()
		, (std::numeric_limits<typename Addr::value_type>::max)());
	return Addr(tmp);
}

template<>
inline boost::uint16_t MaxAddr<boost::uint16_t>()
{ return (std::numeric_limits<boost::uint16_t>::max)(); }

// this is the generic implementation of
// a filter for a specific address type.
// it works with IPv4 and IPv6
template<class Addr, typename DataType>
class FilterImpl
{
public:
    typedef DataType AccessInfo;

public:
	FilterImpl()
	{
		// make the entire ip-range non-blocked
		access_list_.insert(Range(Zero<Addr>(), AccessInfo()));
	}

	void AddRule(Addr first, Addr last, const AccessInfo& access)
	{
		BC_ASSERT(!access_list_.empty());
		BC_ASSERT(first < last || first == last);
		
		typename RangeType::iterator i = access_list_.upper_bound(first);
		typename RangeType::iterator j = access_list_.upper_bound(last);

		if (i != access_list_.begin()) --i;
		
		BC_ASSERT(j != access_list_.begin());
		BC_ASSERT(j != i);
		
		AccessInfo first_access = i->access;
		AccessInfo last_access = boost::prior(j)->access;

		if (i->start != first && first_access != access)
		{
			i = access_list_.insert(i, Range(first, access));
		}
		else if (i != access_list_.begin() && boost::prior(i)->access == access)
		{
			--i;
			first_access = i->access;
		}
		BC_ASSERT(!access_list_.empty());
		BC_ASSERT(i != access_list_.end());

		if (i != j) access_list_.erase(boost::next(i), j);
		if (i->start == first)
		{
			// we can do this const-cast because we know that the new
			// start address will keep the set correctly ordered
			const_cast<Addr&>(i->start) = first;
			const_cast<AccessInfo&>(i->access) = access;
		}
		else if (first_access != access)
		{
			access_list_.insert(i, Range(first, access));
		}
		
		if ((j != access_list_.end()
				&& MinusOne(j->start) != last)
			|| (j == access_list_.end()
				&& last != MaxAddr<Addr>()))
		{
			BC_ASSERT(j == access_list_.end() || last < MinusOne(j->start));
			if (last_access != access)
				j = access_list_.insert(j, Range(PlusOne(last), last_access));
		}

		if (j != access_list_.end() && j->access == access) access_list_.erase(j);
		BC_ASSERT(!access_list_.empty());
	}

	AccessInfo Access(Addr const& addr) const
	{
		BC_ASSERT(!access_list_.empty());
		typename RangeType::const_iterator i = access_list_.upper_bound(addr);
		if (i != access_list_.begin()) --i;
		BC_ASSERT(i != access_list_.end());
		BC_ASSERT(i->start <= addr && (boost::next(i) == access_list_.end()
			|| addr < boost::next(i)->start));
		return i->access;
	}

private:
	struct Range
	{
		Range(Addr addr, AccessInfo a = AccessInfo()): start(addr), access(a) {}
		bool operator<(Range const& r) const
		{ return start < r.start; }
		bool operator<(Addr const& a) const
		{ return start < a; }
		Addr start;
		// the end of the range is implicit
		// and given by the next entry in the set
		AccessInfo access;
	};

	typedef std::set<Range> RangeType;
	RangeType access_list_;
};

}

struct AccessInfo
{
    AccessInfo() : flags(0), upload_speed_limit(0), download_speed_limit(0) {}

    int flags;
    unsigned int upload_speed_limit;
    unsigned int download_speed_limit;
    std::string description; 
};

inline bool operator==(const AccessInfo& lhs, const AccessInfo& rhs)
{
    return (lhs.flags == rhs.flags)
        && (lhs.upload_speed_limit == rhs.upload_speed_limit)
        && (lhs.download_speed_limit == rhs.download_speed_limit)
        && (lhs.description == rhs.description);
}

inline bool operator!=(const AccessInfo& lhs, const AccessInfo& rhs)
{
    return !(lhs == rhs);
}

class IpFilter
{
public:	
	void AddRule(ip_address first, ip_address last, const AccessInfo& access_info);

	AccessInfo Access(ip_address const& addr) const;

	bool Init(std::string const& file); 

    static IpFilter& GetInstance();

private:
    IpFilter() {}
    IpFilter(const IpFilter&);
    IpFilter& operator=(const IpFilter&); 

private:
	Detail::FilterImpl<ipv4_address::bytes_type, AccessInfo> filter_;
};

class PortFilter
{
public:

	enum AccessFlags
	{
		BLOCKED = 1
	};

	void AddRule(boost::uint16_t first, boost::uint16_t last, int access);
	int  Access(boost::uint16_t port) const;

private:
	Detail::FilterImpl<boost::uint16_t, int> portfilter_;
};

}

#endif
