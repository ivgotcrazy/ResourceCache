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

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <boost/bind.hpp>
#include "bencode_entry.hpp"

namespace
{

template <class T>
void call_destructor(T* o)
{
	//BC_ASSERT(o);
	o->~T();
}

std::string ToHex(std::string const& s)
{
	std::string ret;
	char const* digits = "0123456789abcdef";
	for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		ret += digits[((unsigned char)*i) >> 4];
		ret += digits[((unsigned char)*i) & 0xf];
	}
	return ret;
}

}

namespace BroadCache
{
namespace detail
{
	char const* IntegerToStr(char* buf, int size, BencodeEntry::IntegerType val)
	{
		int sign = 0;
		if (val < 0)
		{
			sign = 1;
			val = -val;
		}
		buf[--size] = '\0';
		if (val == 0) buf[--size] = '0';
		for (; size > sign && val != 0;)
		{
			buf[--size] = '0' + char(val % 10);
			val /= 10;
		}
		if (sign) buf[--size] = '-';
		return buf + size;
	}
}

BencodeEntry& BencodeEntry::operator[](char const* key)
{
	DictionaryType::iterator i = Dict().find(key);
	if (i != Dict().end()) return i->second;
	DictionaryType::iterator ret = Dict().insert(
		Dict().begin()
		, std::make_pair(key, BencodeEntry()));
	return ret->second;
}

BencodeEntry& BencodeEntry::operator[](std::string const& key)
{
	DictionaryType::iterator i = Dict().find(key);
	if (i != Dict().end()) return i->second;
	DictionaryType::iterator ret = Dict().insert(
		Dict().begin()
		, std::make_pair(std::string(key), BencodeEntry()));
	return ret->second;
}

BencodeEntry* BencodeEntry::FindKey(char const* key)
{
	DictionaryType::iterator i = Dict().find(key);
	if (i == Dict().end()) return 0;
	return &i->second;
}

BencodeEntry const* BencodeEntry::FindKey(char const* key) const
{
	DictionaryType::const_iterator i = Dict().find(key);
	if (i == Dict().end()) return 0;
	return &i->second;
}
	
BencodeEntry* BencodeEntry::FindKey(std::string const& key)
{
	DictionaryType::iterator i = Dict().find(key);
	if (i == Dict().end()) return 0;
	return &i->second;
}

BencodeEntry const* BencodeEntry::FindKey(std::string const& key) const
{
	DictionaryType::const_iterator i = Dict().find(key);
	if (i == Dict().end()) return 0;
	return &i->second;
}

BencodeEntry::BencodeEntry()
	: type_(UNDEFINED_T)
{
}

BencodeEntry::BencodeEntry(DataType t)
	: type_(UNDEFINED_T)
{
	Construct(t);
}

BencodeEntry::BencodeEntry(const BencodeEntry& e)
	: type_(UNDEFINED_T)
{
	Copy(e);
}

BencodeEntry::BencodeEntry(DictionaryType const& v)
	: type_(UNDEFINED_T)
{
	new(data) DictionaryType(v);
	type_ = DICTIONARY_T;
}

BencodeEntry::BencodeEntry(StringType const& v)
	: type_(UNDEFINED_T)
{
	new(data) StringType(v);
	type_ = STRING_T;
}

BencodeEntry::BencodeEntry(ListType const& v)
	: type_(UNDEFINED_T)
{
	new(data) ListType(v);
	type_ = LIST_T;
}

BencodeEntry::BencodeEntry(IntegerType const& v)
	: type_(UNDEFINED_T)
{
	new(data) IntegerType(v);
	type_ = INT_T;
}

void BencodeEntry::operator=(DictionaryType const& v)
{
	Destruct();
	new(data) DictionaryType(v);
	type_ = DICTIONARY_T;
}

void BencodeEntry::operator=(StringType const& v)
{
	Destruct();
	new(data) StringType(v);
	type_ = STRING_T;
}

void BencodeEntry::operator=(ListType const& v)
{
	Destruct();
	new(data) ListType(v);
	type_ = LIST_T;
}

void BencodeEntry::operator=(IntegerType const& v)
{
	Destruct();
	new(data) IntegerType(v);
	type_ = INT_T;
}

bool BencodeEntry::operator==(BencodeEntry const& e) const
{
	if (type_ != e.type_) return false;

	switch(type_)
	{
	case INT_T:
		return Integer() == e.Integer();
	case STRING_T:
		return String() == e.String();
	case LIST_T:
		return List() == e.List();
	case DICTIONARY_T:
		return Dict() == e.Dict();
	default:
		BC_ASSERT(type_ == UNDEFINED_T);
		return true;
	}
}

void BencodeEntry::Construct(DataType t)
{
	switch(t)
	{
	case INT_T:
		new(data) IntegerType;
		break;
	case STRING_T:
		new(data) StringType;
		break;
	case LIST_T:
		new(data) ListType;
		break;
	case DICTIONARY_T:
		new (data) DictionaryType;
		break;
	default:
		BC_ASSERT(t == UNDEFINED_T);
	}
	type_ = t;
}

void BencodeEntry::Copy(BencodeEntry const& e)
{
	switch (e.Type())
	{
	case INT_T:
		new(data) IntegerType(e.Integer());
		break;
	case STRING_T:
		new(data) StringType(e.String());
		break;
	case LIST_T:
		new(data) ListType(e.List());
		break;
	case DICTIONARY_T:
		new (data) DictionaryType(e.Dict());
		break;
	default:
		BC_ASSERT(e.Type() == UNDEFINED_T);
	}
	type_ = e.Type();
}

void BencodeEntry::Destruct()
{
	switch(type_)
	{
	case INT_T:
		call_destructor(reinterpret_cast<IntegerType*>(data));
		break;
	case STRING_T:
		call_destructor(reinterpret_cast<StringType*>(data));
		break;
	case LIST_T:
		call_destructor(reinterpret_cast<ListType*>(data));
		break;
	case DICTIONARY_T:
		call_destructor(reinterpret_cast<DictionaryType*>(data));
		break;
	default:
		BC_ASSERT(type_ == UNDEFINED_T);
		break;
	}
	type_ = UNDEFINED_T;
}

void BencodeEntry::Swap(BencodeEntry& e)
{
	// not implemented
	BC_ASSERT(false);
}

void BencodeEntry::Print(std::ostream& os, int indent) const
{
	BC_ASSERT(indent >= 0);
	for (int i = 0; i < indent; ++i) os << " ";
	switch (type_)
	{
	case INT_T:
		os << Integer() << "\n";
		break;
	case STRING_T:
		{
			bool binary_string = false;
			for (std::string::const_iterator i = String().begin(); i != String().end(); ++i)
			{
				if (!std::isprint(static_cast<unsigned char>(*i)))
				{
					binary_string = true;
					break;
				}
			}
			if (binary_string) os << ToHex(String()) << "\n";
			else os << String() << "\n";
		} break;
	case LIST_T:
		{
			os << "list\n";
			for (ListType::const_iterator i = List().begin(); i != List().end(); ++i)
			{
				i->Print(os, indent+1);
			}
		} break;
	case DICTIONARY_T:
		{
			os << "dictionary\n";
			for (DictionaryType::const_iterator i = Dict().begin(); i != Dict().end(); ++i)
			{
				bool binary_string = false;
				for (std::string::const_iterator k = i->first.begin(); k != i->first.end(); ++k)
				{
					if (!std::isprint(static_cast<unsigned char>(*k)))
					{
						binary_string = true;
						break;
					}
				}
				for (int j = 0; j < indent+1; ++j) os << " ";
				os << "[";
				if (binary_string) os << ToHex(i->first);
				else os << i->first;
				os << "]";

				if (i->second.Type() != BencodeEntry::STRING_T
					&& i->second.Type() != BencodeEntry::INT_T)
					os << "\n";
				else os << " ";
				i->second.Print(os, indent+2);
			}
		} break;
	default:
		os << "<uninitialized>\n";
	}
}

}
