/*##############################################################################
 * 文件名   : hash_stream.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.10.20
 * 文件描述 : 定义了HashStream类,并重载了&操作符 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_HASH_STREAM
#define HEADER_HASH_STREAM

#include <boost/functional/hash.hpp>

#include "single_value_model.hpp"

namespace BroadCache
{

static const size_t kHashStreamClass = 2;

typedef SingleValueModel<size_t, kHashStreamClass> HashStream;  //定义了哈希流类,用于串行计算对象的hash值

/*------------------------------------------------------------------------------
 * 描  述: 计算一个对象的hash值
 * 参  数: [in][out] stream 哈希流对象
 *         [in] t 计算此对象的hash值
 * 返回值: 哈希流对象
 * 修  改:
 *   时间 2013.10.20
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<typename T>
inline HashStream& operator&(HashStream& stream, const T& t)
{
    boost::hash_combine(stream.value_ref(), t);

    return stream;
}

}

#endif  // HEADER_HASH_STREAM
