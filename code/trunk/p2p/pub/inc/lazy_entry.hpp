/*#############################################################################
 * 文件名   : lazy_entry.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月02日
 * 文件描述 : Bencoding编码声明
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

#ifndef HEADER_LAZY_ENTRY
#define HEADER_LAZY_ENTRY

#include "depend.hpp"
#include "bc_typedef.hpp"


namespace BroadCache
{

class LazyEntry;

int LazyBdecode(char const* start, char const* end, LazyEntry& ret, int depth_limit=1000);

struct LazyDictEntry;

class LazyEntry
{
public:
	enum EntryType
	{
		NONE_ENTRY, DICT_ENTRY, LIST_ENTRY, STRING_ENTRY, INT_ENTRY
	};

	LazyEntry() : begin_(0), len_(0), size_(0), capacity_(0), type_(NONE_ENTRY)
	{
		data_.start = 0;
	}

	EntryType Type() const { return (EntryType)type_; }

	// Int functions

	void ConstructInt(const char* start, int len);
	int64 IntValue() const;

	// String functions

	void ConstructString(const char* start, int len);
	const char* StringPtr() const;
	const char* StringCstr() const;
	std::string StringValue() const;
	int StringLength() const;

	// Dictionary functions

	void ConstructDict(const char* begin);
	LazyEntry* DictAppend(const char* name);
	void Pop();
	LazyEntry* DictFind(const char* name);
	int64 DictFindIntValue(const char* name, int64 default_val = 0) const;
	std::string DictFindStringValue(const char* name) const;
	const LazyEntry* DictFind(const char* name) const;
	const LazyEntry* DictFindInt(const char* name) const;
	const LazyEntry* DictFindDict(const char* name) const;
	const LazyEntry* DictFindList(const char* name) const;
	const LazyEntry* DictFindString(const char* name) const;
	std::pair<std::string, const LazyEntry*> DictAt(int i) const;
	int DictSize() const;

	// List functions

	void ConstructList(const char* begin);
	LazyEntry* ListAppend();
	LazyEntry* ListAt(int i);
	const LazyEntry* ListAt(int i) const;
	std::string ListStringValueAt(int i) const;
	int64 ListIntValueAt(int i, int64 default_val = 0) const;
	int ListSize() const;
	void SetEnd(const char* end);
	void Clear();
	void Release();

	~LazyEntry();

	std::pair<const char*, int> DataSection() const;
	void Swap(LazyEntry& entry);

private:
	union DataType
	{
		LazyDictEntry* dict;
		LazyEntry* list;
		const char* start;
	}data_;

	// used for dictionaries and lists to record the range
	// in the original buffer they are based on
	const char* begin_;

	// the number of bytes this entry extends in the
	// bencoded buffer
	uint32 len_;

	// if list or dictionary, the number of items
	uint32 size_;

	// if list or dictionary, allocated number of items
	uint32 capacity_:29;

	// element type (dict, list, int, string)
	uint32 type_:3;
};

struct LazyDictEntry
{
	const char* name;
	LazyEntry val;
};

std::string PrintEntry(LazyEntry const& e, bool single_line = false, int indent = 0);
std::ostream& operator<<(std::ostream& os, const LazyEntry& entry);

}

#endif


