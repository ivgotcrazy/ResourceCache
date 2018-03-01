/*#############################################################################
 * 文件名   : lazy_entry.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月02日
 * 文件描述 : Bencoding编码定义
 * ##########################################################################*/

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
#include <cstring>
#include <cstdio>
#include "lazy_entry.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

namespace
{
	const int LAZY_ENTRY_GROW_FACTOR = 150; // percent
	const int LAZY_ENTRY_DICT_INIT = 5;
	const int LAZY_ENTRY_LIST_INIT = 5;
}

namespace
{

int FailBdecode(LazyEntry& ret, int return_value = -1)
{
	ret.Clear();
	return return_value;
}

int NumDigits(int val)
{
	int ret = 1;
	while (val >= 10)
	{
		++ret;
		val /= 10;
	}
	return ret;
}

// str1 is null-terminated
// str2 is not, str2 is len2 chars
bool StringEqual(char const* str1, char const* str2, int len2)
{
	while (len2 > 0)
	{
		if (*str1 != *str2) return false;
		if (*str1 == 0) return false;
		++str1;
		++str2;
		--len2;
	}
	return *str1 == 0;
}

int LineLongerThan(LazyEntry const& e, int limit)
{
	int line_len = 0;
	switch (e.Type())
	{
	case LazyEntry::LIST_ENTRY:
		line_len += 4;
		if (line_len > limit) return -1;
		for (int i = 0; i < e.ListSize(); ++i)
		{
			int ret = LineLongerThan(*e.ListAt(i), limit - line_len);
			if (ret == -1) return -1;
			line_len += ret + 2;
		}
		break;
	case LazyEntry::DICT_ENTRY:
		line_len += 4;
		if (line_len > limit) return -1;
		for (int i = 0; i < e.DictSize(); ++i)
		{
			line_len += 4 + e.DictAt(i).first.size();
			if (line_len > limit) return -1;
			int ret = LineLongerThan(*e.DictAt(i).second, limit - line_len);
			if (ret == -1) return -1;
			line_len += ret + 1;
		}
		break;
	case LazyEntry::STRING_ENTRY:
		line_len += 3 + e.StringLength();
		break;
	case LazyEntry::INT_ENTRY:
	{
		int64 val = e.IntValue();
		while (val > 0)
		{
			++line_len;
			val /= 10;
		}
		line_len += 2;
	}
	break;
	case LazyEntry::NONE_ENTRY:
		line_len += 4;
		break;
	}
	
	if (line_len > limit) return -1;
	return line_len;
}

}

//====================================Int======================================

// fills in 'val' with what the string between start and the
// first occurance of the delimiter is interpreted as an int.
// return the pointer to the delimiter, or 0 if there is a
// parse error. val should be initialized to zero
char const* ParseInt(char const* start, char const* end, char delimiter, boost::int64_t& val)
{
	while (start < end && *start != delimiter)
	{
		if (!IsDigit(*start)) { return 0; }
		val *= 10;
		val += *start - '0';
		++start;
	}
	return start;
}

char const* FindChar(char const* start, char const* end, char delimiter)
{
	while (start < end && *start != delimiter) ++start;
	return start;
}

// return 0 = success
int LazyBdecode(char const* start, char const* end, LazyEntry& ret, int depth_limit)
{
	ret.Clear();
	if (start == end) return 0;

	std::vector<LazyEntry*> stack;

	stack.push_back(&ret);
	while (start < end)
	{
		if (stack.empty()) break; // done!

		LazyEntry* top = stack.back();

		if (int(stack.size()) > depth_limit) return FailBdecode(ret);
		if (start >= end) return FailBdecode(ret);
		char t = *start;
		++start;
		if (start >= end && t != 'e') return FailBdecode(ret);

		switch (top->Type())
		{
			case LazyEntry::DICT_ENTRY:
			{
				if (t == 'e')
				{
					top->SetEnd(start);
					stack.pop_back();
					continue;
				}
				if (!IsDigit(t)) return FailBdecode(ret);
				boost::int64_t len = t - '0';
				start = ParseInt(start, end, ':', len);
				if (start == 0 || start + len + 3 > end || *start != ':') return FailBdecode(ret);
				++start;
				if (start == end) return FailBdecode(ret);
				LazyEntry* ent = top->DictAppend(start);
				if (ent == 0) return FailBdecode(ret, -2);
				start += len;
				if (start >= end) return FailBdecode(ret);
				stack.push_back(ent);
				t = *start;
				++start;
				break;
			}
			case LazyEntry::LIST_ENTRY:
			{
				if (t == 'e')
				{
					top->SetEnd(start);
					stack.pop_back();
					continue;
				}
				LazyEntry* ent = top->ListAppend();
				if (ent == 0) return FailBdecode(ret, -2);
				stack.push_back(ent);
				break;
			}
			default: break;
		}

		top = stack.back();
		switch (t)
		{
			case 'd':
				top->ConstructDict(start - 1);
				continue;
			case 'l':
				top->ConstructList(start - 1);
				continue;
			case 'i':
			{
				char const* int_start = start;
				start = FindChar(start, end, 'e');
				top->ConstructInt(int_start, start - int_start);
				if (start == end) return FailBdecode(ret);
				BC_ASSERT(*start == 'e');
				++start;
				stack.pop_back();
				continue;
			}
			default:
			{
				if (!IsDigit(t)) return FailBdecode(ret);

				boost::int64_t len = t - '0';
				start = ParseInt(start, end, ':', len);
				if (start == 0 || start + len + 1 > end || *start != ':') return FailBdecode(ret);
				++start;
				top->ConstructString(start, int(len));
				stack.pop_back();
				start += len;
				continue;
			}
		}
		return 0;
	}
	return 0;
}

void LazyEntry::ConstructInt(const char* start, int len)
{
	BC_ASSERT(type_ == NONE_ENTRY);
	type_ = INT_ENTRY;
	data_.start = start;
	size_ = len;
	begin_ = start - 1; // include 'i'
	len_ = len + 2; // include 'e'
}

int64 LazyEntry::IntValue() const
{
	BC_ASSERT(type_ == INT_ENTRY);
	boost::int64_t val = 0;
	bool negative = false;
	if (*data_.start == '-') negative = true;
	ParseInt(negative ? (data_.start + 1) : data_.start, data_.start + size_, 'e', val);
	if (negative) val = -val;
	return val;
}

//==================================String=====================================

void LazyEntry::ConstructString(const char* start, int len)
{
	BC_ASSERT(type_ == NONE_ENTRY);
	type_ = STRING_ENTRY;
	data_.start = start;
	size_ = len;
	begin_ = start - 1 - NumDigits(len);
	len_ = start - begin_ + len;
}

const char* LazyEntry::StringPtr() const
{
	BC_ASSERT(type_ == STRING_ENTRY);
	return data_.start;
}

const char* LazyEntry::StringCstr() const
{
	BC_ASSERT(type_ == STRING_ENTRY);
	const_cast<char*>(data_.start)[size_] = 0;
	return data_.start;
}

std::string LazyEntry::StringValue() const
{
	BC_ASSERT(type_ == STRING_ENTRY);
	return std::string(data_.start, size_);
}

int LazyEntry::StringLength() const
{
	return size_;
}

//===================================Dict======================================

void LazyEntry::ConstructDict(const char* begin)
{
	BC_ASSERT(type_ == NONE_ENTRY);
	type_ = DICT_ENTRY;
	size_ = 0;
	capacity_ = 0;
	begin_ = begin;
}

LazyEntry* LazyEntry::DictAppend(const char* name)
{
	BC_ASSERT(type_ == DICT_ENTRY);
	BC_ASSERT(size_ <= capacity_);
	if (capacity_ == 0)
	{
		int capacity = LAZY_ENTRY_DICT_INIT;
		data_.dict = new (std::nothrow) LazyDictEntry[capacity];
		if (data_.dict == 0) return 0;
		capacity_ = capacity;
	}
	else if (size_ == capacity_)
	{
		int capacity = capacity_ * LAZY_ENTRY_GROW_FACTOR / 100;
		LazyDictEntry* tmp = new (std::nothrow) LazyDictEntry[capacity];
		if (tmp == 0) return 0;
		std::memcpy(tmp, data_.dict, sizeof(LazyDictEntry) * size_);
		for (int i = 0; i < int(size_); ++i) data_.dict[i].val.Release();
		delete[] data_.dict;
		data_.dict = tmp;
		capacity_ = capacity;
	}

	BC_ASSERT(size_ < capacity_);
	LazyDictEntry& ret = data_.dict[size_++];
	ret.name = name;
	return &ret.val;
}

void LazyEntry::Pop()
{
	if (size_ > 0) --size_;
}

LazyEntry* LazyEntry::DictFind(const char* name)
{
	BC_ASSERT(type_ == DICT_ENTRY);
	for (int i = 0; i < int(size_); ++i)
	{
		LazyDictEntry& e = data_.dict[i];
		if (StringEqual(name, e.name, e.val.begin_ - e.name))
			return &e.val;
	}
	return 0;
}

int64 LazyEntry::DictFindIntValue(const char* name, int64 default_val) const
{
	LazyEntry const* e = DictFind(name);
	if (e == 0 || e->Type() != LazyEntry::INT_ENTRY) return default_val;
	return e->IntValue();
}

std::string LazyEntry::DictFindStringValue(const char* name) const
{
	LazyEntry const* e = DictFind(name);
	if (e == 0 || e->Type() != LazyEntry::STRING_ENTRY) return std::string();
	return e->StringValue();
}

const LazyEntry* LazyEntry::DictFind(const char* name) const
{
	return const_cast<LazyEntry*>(this)->DictFind(name);
}

const LazyEntry* LazyEntry::DictFindInt(const char* name) const
{
	LazyEntry const* e = DictFind(name);
	if (e == 0 || e->Type() != LazyEntry::INT_ENTRY) return 0;
	return e;
}

const LazyEntry* LazyEntry::DictFindDict(const char* name) const
{
	LazyEntry const* e = DictFind(name);
	if (e == 0 || e->Type() != LazyEntry::DICT_ENTRY) return 0;
	return e;
}

const LazyEntry* LazyEntry::DictFindList(const char* name) const
{
	LazyEntry const* e = DictFind(name);
	if (e == 0 || e->Type() != LazyEntry::LIST_ENTRY) return 0;
	return e;
}

const LazyEntry* LazyEntry::DictFindString(const char* name) const
{
	LazyEntry const* e = DictFind(name);
	if (e == 0 || e->Type() != LazyEntry::STRING_ENTRY) return 0;
	return e;
}

std::pair<std::string, const LazyEntry*> LazyEntry::DictAt(int i) const
{
	BC_ASSERT(type_ == DICT_ENTRY);
	BC_ASSERT(i < int(size_));
	LazyDictEntry const& e = data_.dict[i];
	return std::make_pair(std::string(e.name, e.val.begin_ - e.name), &e.val);
}

int LazyEntry::DictSize() const
{
	BC_ASSERT(type_ == DICT_ENTRY);
	return size_;
}

//===================================List======================================

void LazyEntry::ConstructList(const char* begin)
{
	BC_ASSERT(type_ == NONE_ENTRY);
	type_ = LIST_ENTRY;
	size_ = 0;
	capacity_ = 0;
	begin_ = begin;
}

LazyEntry* LazyEntry::ListAppend()
{
	BC_ASSERT(type_ == LIST_ENTRY);
	BC_ASSERT(size_ <= capacity_);
	if (capacity_ == 0)
	{
		int capacity = LAZY_ENTRY_LIST_INIT;
		data_.list = new (std::nothrow) LazyEntry[capacity];
		if (data_.list == 0) return 0;
		capacity_ = capacity;
	}
	else if (size_ == capacity_)
	{
		int capacity = capacity_ * LAZY_ENTRY_GROW_FACTOR / 100;
		LazyEntry* tmp = new (std::nothrow) LazyEntry[capacity];
		if (tmp == 0) return 0;
		std::memcpy(tmp, data_.list, sizeof(LazyEntry) * size_);
		for (int i = 0; i < int(size_); ++i) data_.list[i].Release();
		delete[] data_.list;
		data_.list = tmp;
		capacity_ = capacity;
	}

	BC_ASSERT(size_ < capacity_);
	return data_.list + (size_++);
}

LazyEntry* LazyEntry::ListAt(int i)
{
	BC_ASSERT(type_ == LIST_ENTRY);
	BC_ASSERT(i < int(size_));
	return &data_.list[i];
}

const LazyEntry* LazyEntry::ListAt(int i) const
{
	return const_cast<LazyEntry*>(this)->ListAt(i);
}

std::string LazyEntry::ListStringValueAt(int i) const
{
	LazyEntry const* e = ListAt(i);
	if (e == 0 || e->Type() != LazyEntry::STRING_ENTRY) return std::string();
	return e->StringValue();
}

int64 LazyEntry::ListIntValueAt(int i, int64 default_val) const
{
	LazyEntry const* e = ListAt(i);
	if (e == 0 || e->Type() != LazyEntry::INT_ENTRY) return default_val;
	return e->IntValue();
}

int LazyEntry::ListSize() const
{
	BC_ASSERT(type_ == LIST_ENTRY);
	return int(size_);
}

void LazyEntry::SetEnd(const char* end)
{
	BC_ASSERT(end > begin_);
	len_ = end - begin_;
}

void LazyEntry::Clear()
{
	switch (type_)
	{
		case LIST_ENTRY: delete[] data_.list; break;
		case DICT_ENTRY: delete[] data_.dict; break;
		default: break;
	}
	data_.start = 0;
	size_ = 0;
	capacity_ = 0;
	type_ = NONE_ENTRY;
}

void LazyEntry::Release()
{
	data_.start = 0;
	size_ = 0;
	capacity_ = 0;
	type_ = NONE_ENTRY;
}

LazyEntry::~LazyEntry()
{
	Clear();
}

std::pair<const char*, int> LazyEntry::DataSection() const
{
	typedef std::pair<char const*, int> return_t;
	return return_t(begin_, len_);
}

void LazyEntry::Swap(LazyEntry& entry)
{
	using std::swap;
	boost::uint32_t tmp = entry.type_;
	entry.type_ = type_;
	type_ = tmp;
	tmp = entry.capacity_;
	entry.capacity_ = capacity_;
	capacity_ = tmp;
	swap(data_.start, entry.data_.start);
	swap(size_, entry.size_);
	swap(begin_, entry.begin_);
	swap(len_, entry.len_);
}

std::string PrintEntry(LazyEntry const& e, bool single_line, int indent)
{
	char indent_str[200];
	memset(indent_str, ' ', 200);
	indent_str[0] = ',';
	indent_str[1] = '\n';
	indent_str[199] = 0;
	if (indent < 197 && indent >= 0) indent_str[indent+2] = 0;
	std::string ret;
	switch (e.Type())
	{
		case LazyEntry::NONE_ENTRY: return "none";
		case LazyEntry::INT_ENTRY:
		{
			char str[100];
			snprintf(str, sizeof(str), "%ld", e.IntValue());
			return str;
		}
		case LazyEntry::STRING_ENTRY:
		{
			bool printable = true;
			char const* str = e.StringPtr();
			for (int i = 0; i < e.StringLength(); ++i)
			{
				using namespace std;
				if (IsPrint((unsigned char)str[i])) continue;
				printable = false;
				break;
			}
			ret += "'";
			if (printable)
			{
				ret += e.StringValue();
				ret += "'";
				return ret;
			}
			for (int i = 0; i < e.StringLength(); ++i)
			{
				char tmp[5];
				snprintf(tmp, sizeof(tmp), "%02x", (unsigned char)str[i]);
				ret += tmp;
			}
			ret += "'";
			return ret;
		}
		case LazyEntry::LIST_ENTRY:
		{
			ret += '[';
			bool one_liner = LineLongerThan(e, 200) != -1 || single_line;

			if (!one_liner) ret += indent_str + 1;
			for (int i = 0; i < e.ListSize(); ++i)
			{
				if (i == 0 && one_liner) ret += " ";
				ret += PrintEntry(*e.ListAt(i), single_line, indent + 2);
				if (i < e.ListSize() - 1) ret += (one_liner?", ":indent_str);
				else ret += (one_liner?" ":indent_str+1);
			}
			ret += "]";
			return ret;
		}
		case LazyEntry::DICT_ENTRY:
		{
			ret += "{";
			bool one_liner = LineLongerThan(e, 200) != -1 || single_line;

			if (!one_liner) ret += indent_str+1;
			for (int i = 0; i < e.DictSize(); ++i)
			{
				if (i == 0 && one_liner) ret += " ";
				std::pair<std::string, LazyEntry const*> ent = e.DictAt(i);
				ret += "'";
				ret += ent.first;
				ret += "': ";
				ret += PrintEntry(*ent.second, single_line, indent + 2);
				if (i < e.DictSize() - 1) ret += (one_liner?", ":indent_str);
				else ret += (one_liner?" ":indent_str+1);
			}
			ret += "}";
			return ret;
		}
	}

	return ret;
}

std::ostream& operator<<(std::ostream& os, const LazyEntry& entry)
{
	return os << PrintEntry(entry);
}

}