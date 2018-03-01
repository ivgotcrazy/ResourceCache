/*#############################################################################
 * 文件名   : bc_io.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月24日
 * 文件描述 : 网络字节序读写接口
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BC_IO
#define HEADER_BC_IO

#include <string>
#include <boost/type.hpp>

namespace BroadCache
{

namespace IO
{

template <class T> struct type {};

// reads an integer from a byte stream
// in big endian byte order and converts
// it to native endianess
template <class T, class InIt>
inline T read_impl(InIt& start, type<T>)
{
	T ret = 0;
	for (int i = 0; i < (int)sizeof(T); ++i)
	{
		ret <<= 8;
		ret |= static_cast<boost::uint8_t>(*start);
		++start;
	}
	return ret;
}

template <class InIt>
boost::uint8_t read_impl(InIt& start, type<boost::uint8_t>)
{
	return static_cast<boost::uint8_t>(*start++);
}

template <class InIt>
boost::int8_t read_impl(InIt& start, type<boost::int8_t>)
{
	return static_cast<boost::int8_t>(*start++);
}

template <class T, class OutIt>
inline void write_impl(T val, OutIt& start)
{
	for (int i = (int)sizeof(T)-1; i >= 0; --i)
	{
		*start = static_cast<unsigned char>((val >> (i * 8)) & 0xff);
		++start;
	}
}

// -- adaptors

template <class InIt>
boost::int64_t read_int64(InIt& start)
{ return read_impl(start, type<boost::int64_t>()); }

template <class InIt>
boost::uint64_t read_uint64(InIt& start)
{ return read_impl(start, type<boost::uint64_t>()); }

template <class InIt>
boost::uint32_t read_uint32(InIt& start)
{ return read_impl(start, type<boost::uint32_t>()); }

template <class InIt>
boost::int32_t read_int32(InIt& start)
{ return read_impl(start, type<boost::int32_t>()); }

template <class InIt>
boost::int16_t read_int16(InIt& start)
{ return read_impl(start, type<boost::int16_t>()); }

template <class InIt>
boost::uint16_t read_uint16(InIt& start)
{ return read_impl(start, type<boost::uint16_t>()); }

template <class InIt>
boost::int8_t read_int8(InIt& start)
{ return read_impl(start, type<boost::int8_t>()); }

template <class InIt>
boost::uint8_t read_uint8(InIt& start)
{ return read_impl(start, type<boost::uint8_t>()); }


template <class OutIt>
void write_uint64(boost::uint64_t val, OutIt& start)
{ write_impl(val, start); }

template <class OutIt>
void write_int64(boost::int64_t val, OutIt& start)
{ write_impl(val, start); }

template <class OutIt>
void write_uint32(boost::uint32_t val, OutIt& start)
{ write_impl(val, start); }

template <class OutIt>
void write_int32(boost::int32_t val, OutIt& start)
{ write_impl(val, start); }

template <class OutIt>
void write_uint16(boost::uint16_t val, OutIt& start)
{ write_impl(val, start); }

template <class OutIt>
void write_int16(boost::int16_t val, OutIt& start)
{ write_impl(val, start); }

template <class OutIt>
void write_uint8(boost::uint8_t val, OutIt& start)
{ write_impl(val, start); }

template <class OutIt>
void write_int8(boost::int8_t val, OutIt& start)
{ write_impl(val, start); }

inline void write_string(std::string const& str, char*& start)
{
	std::memcpy((void*)start, str.c_str(), str.size());
	start += str.size();
}

template <class OutIt>
void write_string(std::string const& str, OutIt& start)
{
	std::copy(str.begin(), str.end(), start);
}

//---------------------------------------  Direct Deal ----------------------
// reads an integer from a byte stream
// in big endian byte order and don't 
//converts
template <class T, class InIt>
inline T read_impl_be(InIt& start, type<T>)
{
	T ret = 0;
	for (int i = 0; i < (int)sizeof(T); ++i)
	{
	  ret |= static_cast<unsigned char>(*start) << (8 * i);
	  ++start;
	}
	return ret;
}

template <class T, class OutIt>
static inline void write_impl_be(T val, OutIt& start)
{
	for (int i = 0; i < (int)sizeof(T); ++i)
	{
	  *start = static_cast<unsigned char>((val >> (i * 8)) & 0xff);
	  ++start;
	}
}

// -- adaptors

template <class InIt>
boost::int64_t read_int16_be(InIt& start)
{ return read_impl_be(start, type<boost::int16_t>()); }

template <class InIt>
boost::int64_t read_uint16_be(InIt& start)
{ return read_impl_be(start, type<boost::uint16_t>()); }


template <class InIt>
boost::uint32_t read_uint32_be(InIt& start)
{ return read_impl_be(start, type<boost::uint32_t>()); }


template <class OutIt>
void write_uint16_be(boost::uint16_t val, OutIt& start)
{ write_impl_be(val, start); }

template <class OutIt>
void write_uint32_be(boost::uint32_t val, OutIt& start)
{ write_impl_be(val, start); }

} // IO

} // BroadCache

#endif
