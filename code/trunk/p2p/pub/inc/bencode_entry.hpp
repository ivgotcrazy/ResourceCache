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

#ifndef HEADER_BENCODE_ENTRY
#define HEADER_BENCODE_ENTRY

/*
 *
 * This file declares the entry class. It is a
 * variant-type that can be an integer, list,
 * dictionary (map) or a string. This type is
 * used to hold bdecoded data (which is the
 * encoding BitTorrent messages uses).
 *
 * it has 4 accessors to access the actual
 * type of the object. They are:
 * integer()
 * string()
 * list()
 * dict()
 * The actual type has to match the type you
 * are asking for, otherwise you will get an
 * assertion failure.
 * When you default construct an entry, it is
 * uninitialized. You can initialize it through the
 * assignment operator, copy-constructor or
 * the constructor that takes a data_type enum.
 *
 *
 */


#include <iosfwd>
#include <map>
#include <list>
#include <string>
#include <stdexcept>
#include "bc_typedef.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

namespace detail
{

template<int v1, int v2>
struct max2 
{ 
	enum 
	{ 
		value = v1 > v2 ? v1 : v2 
	}; 
};

template<int v1, int v2, int v3>
struct max3
{
	enum
	{
		temp = max2<v1,v2>::value,
		value = temp>v3?temp:v3
	};
};

template<int v1, int v2, int v3, int v4>
struct max4
{
	enum
	{
		temp = max3<v1,v2, v3>::value,
		value = temp>v4?temp:v4
	};
};

}


class BencodeEntry
{
public:

	// the key is always a string. If a generic entry would be allowed
	// as a key, sorting would become a problem (e.g. to compare a string
	// to a list). The definition doesn't mention such a limit though.
	typedef std::map<std::string, BencodeEntry> DictionaryType;
	typedef std::string StringType;
	typedef std::list<BencodeEntry> ListType;
	typedef size_type IntegerType;

	enum DataType
	{
		INT_T,
		STRING_T,
		LIST_T,
		DICTIONARY_T,
		UNDEFINED_T
	};

	DataType Type() const;

	BencodeEntry(DictionaryType const&);
	BencodeEntry(StringType const&);
	BencodeEntry(ListType const&);
	BencodeEntry(IntegerType const&);

	BencodeEntry();
	BencodeEntry(DataType t);
	BencodeEntry(BencodeEntry const& e);
	~BencodeEntry();

	bool operator==(BencodeEntry const& e) const;
		
	void operator=(BencodeEntry const&);
	void operator=(DictionaryType const&);
	void operator=(StringType const&);
	void operator=(ListType const&);
	void operator=(IntegerType const&);

	IntegerType& Integer();
	const IntegerType& Integer() const;
	StringType& String();
	const StringType& String() const;
	ListType& List();
	const ListType& List() const;
	DictionaryType& Dict();
	const DictionaryType& Dict() const;

	void Swap(BencodeEntry& e);

	// these functions requires that the entry
	// is a dictionary, otherwise they will throw	
	BencodeEntry& operator[](char const* key);
	BencodeEntry& operator[](std::string const& key);
	BencodeEntry* FindKey(char const* key);
	BencodeEntry const* FindKey(char const* key) const;
	BencodeEntry* FindKey(std::string const& key);
	BencodeEntry const* FindKey(std::string const& key) const;
		
	void Print(std::ostream& os, int indent = 0) const;

protected:

	void Construct(DataType t);
	void Copy(const BencodeEntry& e);
	void Destruct();

private:

	DataType type_;

	union
	{
		char data[detail::max4<sizeof(ListType)
			, sizeof(DictionaryType)
			, sizeof(StringType)
			, sizeof(IntegerType)>::value];
		IntegerType dummy_aligner;
	};
};

inline std::ostream& operator<<(std::ostream& os, const BencodeEntry& e)
{
	e.Print(os, 0);
	return os;
}

inline BencodeEntry::DataType BencodeEntry::Type() const
{
	return type_;
}

inline BencodeEntry::~BencodeEntry() { Destruct(); }

inline void BencodeEntry::operator=(const BencodeEntry& e)
{
	Destruct();
	Copy(e);
}

inline BencodeEntry::IntegerType& BencodeEntry::Integer()
{
	if (type_ == UNDEFINED_T) Construct(INT_T);
	BC_ASSERT(type_ == INT_T);
	return *reinterpret_cast<IntegerType*>(data);
}

inline BencodeEntry::IntegerType const& BencodeEntry::Integer() const
{
	BC_ASSERT(type_ == INT_T);
	return *reinterpret_cast<const IntegerType*>(data);
}

inline BencodeEntry::StringType& BencodeEntry::String()
{   
	if (type_ == UNDEFINED_T) Construct(STRING_T);
	BC_ASSERT(type_ == STRING_T);
	return *reinterpret_cast<StringType*>(data);
}

inline BencodeEntry::StringType const& BencodeEntry::String() const
{
	BC_ASSERT(type_ == STRING_T);
	return *reinterpret_cast<const StringType*>(data);
}

inline BencodeEntry::ListType& BencodeEntry::List()
{
	if (type_ == UNDEFINED_T) Construct(LIST_T);
	BC_ASSERT(type_ == LIST_T);
	return *reinterpret_cast<ListType*>(data);
}

inline BencodeEntry::ListType const& BencodeEntry::List() const
{
	BC_ASSERT(type_ == LIST_T);
	return *reinterpret_cast<const ListType*>(data);
}

inline BencodeEntry::DictionaryType& BencodeEntry::Dict()
{
	if (type_ == UNDEFINED_T) Construct(DICTIONARY_T);
	BC_ASSERT(type_ == DICTIONARY_T);
	return *reinterpret_cast<DictionaryType*>(data);
}

inline BencodeEntry::DictionaryType const& BencodeEntry::Dict() const
{
	BC_ASSERT(type_ == DICTIONARY_T);
	return *reinterpret_cast<const DictionaryType*>(data);
}

}

#endif // HEADER_BENCODE_ENTRY
