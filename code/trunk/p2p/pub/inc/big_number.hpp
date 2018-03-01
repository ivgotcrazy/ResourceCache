/*

Copyright (c) 2003, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef HEADER_BIG_NUMBER
#define HEADER_BIG_NUMBER

#include <iostream>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <string>
#include <cstring>

#include "bc_assert.hpp"

namespace
{
bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}
}

namespace BroadCache
{

/******************************************************************************
 * 描述：从libtorrent移植，将BigNumber的字节个数参数化，使其能够适应不同协议的
 *       的变化。需要注意的是，不同长度的BigNumber不能进行混合操作，当前对于这
 *       种操作有做assert处理，但是在release版本中没有做任何判断，如果存在混合
 *       操作，则release版本很可能出现内存越界等严重问题。
 * 作者：teck_zhou
 * 时间：2013/08/28
 *****************************************************************************/
class BigNumber
{
public:
	/*explicit BigNumber(int number_size) : number_size_(number_size)
	{
		BC_ASSERT(number_size != 0);
		number_ = new unsigned char[number_size];
		Clear();
	}

	BigNumber(int number_size, char const* s) : number_size_(number_size)
	{
        BC_ASSERT(number_size != 0);
        number_ = new 
		if (s == 0) 
		{
			Clear();
		}
		else
		{
			std::memcpy(number_, s, number_size);
		}
	}
    */

    explicit BigNumber(int number_size, char const* s = nullptr) : number_size_(number_size)
    {
        BC_ASSERT(number_size != 0);
        number_ = new unsigned char[number_size];
        if(s != nullptr)
        {
            std::memcpy(number_, s, number_size);
        }
        else
        {
            Clear();
        } 
    }

	~BigNumber()
	{
		delete[] number_;
		number_ = 0;
		number_size_ = 0;
	}

	void Assign(std::string const& s)
	{
		BC_ASSERT(s.size() >= number_size_);
		uint32 sl = s.size() < number_size_ ? s.size() : number_size_;
		std::memcpy(number_, &s[0], sl);
	}

	void Clear()
	{
		std::fill(number_, number_ + number_size_, 0);
	}

	bool IsAllZeros() const
	{
		return std::count(number_, number_ + number_size_, 0) == number_size_;
	}

	bool operator==(BigNumber const& n) const
	{
		BC_ASSERT(n.number_size_ == number_size_);
		return std::equal(n.number_, n.number_ + number_size_, number_);
	}

	bool operator!=(BigNumber const& n) const
	{
		BC_ASSERT(n.number_size_ == number_size_);
		return !std::equal(n.number_, n.number_ + number_size_, number_);
	}

	bool operator<(BigNumber const& n) const
	{
		BC_ASSERT(n.number_size_ == number_size_);
		for (uint32 i = 0; i < number_size_; ++i)
		{
			if (number_[i] < n.number_[i]) return true;
			if (number_[i] > n.number_[i]) return false;
		}
		return false;
	}
		
	BigNumber operator~()
	{
		BigNumber ret(number_size_);
		for (uint32 i = 0; i< number_size_; ++i)
			ret.number_[i] = ~number_[i];
		return ret;
	}
		
	BigNumber& operator &= (BigNumber const& n)
	{
		BC_ASSERT(n.number_size_ == number_size_);
		for (uint32 i = 0; i< number_size_; ++i)
			number_[i] &= n.number_[i];
		return *this;
	}

	BigNumber& operator |= (BigNumber const& n)
	{
		BC_ASSERT(n.number_size_ == number_size_);
		for (uint32 i = 0; i< number_size_; ++i)
			number_[i] |= n.number_[i];
		return *this;
	}

	BigNumber& operator ^= (BigNumber const& n)
	{
		BC_ASSERT(n.number_size_ == number_size_);
		for (uint32 i = 0; i< number_size_; ++i)
			number_[i] ^= n.number_[i];
		return *this;
	}
		
	unsigned char& operator[](uint32 i)
	{ BC_ASSERT(i >= 0 && i < number_size_); return number_[i]; }

	unsigned char const& operator[](uint32 i) const
	{ BC_ASSERT(i >= 0 && i < number_size_); return number_[i]; }

	typedef const unsigned char* const_iterator;
	typedef unsigned char* iterator;

	const_iterator begin() const { return number_; }
	const_iterator end() const { return number_ + number_size_; }

	iterator begin() { return number_; }
	iterator end() { return number_ + number_size_; }

	std::string to_string() const
	{ return std::string((char const*)&number_[0], number_size_); }

    uint32 size() const
    {
        return static_cast<uint32>(number_size_);
    }

private:
	unsigned char* number_;
	uint32 number_size_;
};

inline std::ostream& operator<<(std::ostream& os, BigNumber const& big_number)
{
	for (BigNumber::const_iterator i = big_number.begin();
		i != big_number.end(); ++i)
	{
		os << std::hex << std::setw(2) << std::setfill('0')
			<< static_cast<unsigned int>(*i);
	}
	os << std::dec << std::setfill(' ');
	return os;
}

inline std::istream& operator>>(std::istream& is, BigNumber& big_number)
{
	for (BigNumber::iterator i = big_number.begin();
		i != big_number.end(); ++i)
	{
		char c[2];
		is >> c[0] >> c[1];
		c[0] = tolower(c[0]);
		c[1] = tolower(c[1]);
		if (
			((c[0] < '0' || c[0] > '9') && (c[0] < 'a' || c[0] > 'f'))
			|| ((c[1] < '0' || c[1] > '9') && (c[1] < 'a' || c[1] > 'f'))
			|| is.fail())
		{
			is.setstate(std::ios_base::failbit);
			return is;
		}
		*i = ((is_digit(c[0])?c[0]-'0':c[0]-'a'+10) << 4)
			+ (is_digit(c[1])?c[1]-'0':c[1]-'a'+10);
	}
	return is;
}

}

#endif // TORRENT_PEER_ID_HPP_INCLUDED
