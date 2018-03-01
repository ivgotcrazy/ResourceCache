/*------------------------------------------------------------------------------
 * 文件名   : bencode.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.09.13
 * 文件描述 : bencode编码/解码的声明头文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_BENCODE
#define HEADER_BENCODE

#include "bencode_entry.hpp"

namespace BroadCache
{

inline std::string Bencode(const BencodeEntry& entry);

BencodeEntry Bdecode(const std::string& data);
BencodeEntry Bdecode(const char* begin, const char* end, size_t* len = nullptr);

}

#include "bencode_impl.hpp"

#endif  // HEADER_BENCODE
