/**************************************************************************
 * 文件名   : net_byte_order_impl.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.10.07
 * 文件描述 : 此文件是net_byte_order.hpp的实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 *************************************************************************/
#ifndef HEADER_NET_BYTE_ORDER_IMPL
#define HEADER_NET_BYTE_ORDER_IMPL

#include <cstring>
#include <asm/byteorder.h>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: NetSerializer类重载&操作符
 * 参  数: [in][out]serializer 网络流对象
 *         [in]t 被写到网络流中的数据
 * 返回值: 网络流对象
 * 修  改:
 *   时间 2013.10.07
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<typename T>
inline NetSerializer& operator&(NetSerializer& serializer, const T& t)
{
    serializer.set_value(HostToNet(serializer.value(), t));

    return serializer;
}

/*------------------------------------------------------------------------------
 * 描  述: NetUnserializer类重载&操作符
 * 参  数: [in][out]unserializer 网络流对象
 *         [out]t 从网络流中解析出来的数据
 * 返回值: 网络流对象
 * 修  改:
 *   时间 2013.10.07
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<typename T>
inline NetUnserializer& operator&(NetUnserializer& unserializer, T& t)
{
    unserializer.set_value(NetToHost(unserializer.value(), t));

    return unserializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 将网络字节顺序的数转换为本地字节顺序
 * 参  数: [in] t 网络字节顺序的数
 * 返回值: 本地字节顺序的数
 * 修  改:
 *   时间 2013.11.18
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<typename T>
inline T NetToHost(const T& t)
{
    return NetToHost<T>(reinterpret_cast<const char*>(&t)); 
}

/**************************************************************************
 * 描  述: 从网络缓冲区中读取一个类型为T的对象
 * 参  数: [in]buf 网络缓冲区
 * 返回值: 读取类型为T的对象
 * 修  改:
 *   时间 2013.10.07
 *   作者 rosan
 *   描述 创建
 *************************************************************************/
template<typename T>
inline T NetToHost(const char* buf)
{
    T t;
    NetToHost(buf, t);

    return t;
}

/**************************************************************************
 * 描  述: 从网络缓冲区中读取一个类型为T的对象
 * 参  数: [in]buf 网络缓冲区
 *         [out]t 读取到的类型为T的对象
 * 返回值: 读取类型为T的对象后，网络缓冲区指针新的指向
 * 修  改:
 *   时间 2013.10.07
 *   作者 rosan
 *   描述 创建
 *************************************************************************/
template<typename T>
inline const char* NetToHost(const char* buf, T& t)
{
#if defined(__LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN_BITFIELD) //小端系统
    char* p = reinterpret_cast<char*>(&t + 1); //定位到最后一个字节的下一字节
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        *(--p) = *(buf++); 
    }
#elif defined(__BIG_ENDIAN) || defined(__BIG_ENDIAN_BITFIELD) //大端系统
    memcpy(&t, buf, sizeof(T));
    buf += sizeof(T);
#else
#error "no endian defined"
#endif

    return buf;
}

/*------------------------------------------------------------------------------
 * 描  述: 实现数值从本地字节顺序向网络字节顺序转换
 * 参  数: [in] t 本地字节顺序的数
 * 返回值: 网络字节顺序的数
 * 修  改:
 *   时间 2013.11.18
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<typename T>
inline T HostToNet(const T& t)
{
    T ret;
    HostToNet<T>(reinterpret_cast<char*>(&ret), t);

    return ret;
}

/**************************************************************************
 * 描  述: 将本地数据写到网络缓冲区中
 * 参  数: [in][out]buf 网络缓冲区
 *         [in]t 被写到网络缓冲区中的对象
 * 返回值: 写入类型为T的对象后网络缓冲区新的指向
 * 修  改:
 *   时间 2013.10.07
 *   作者 rosan
 *   描述 创建
 *************************************************************************/
template<typename T>
inline char* HostToNet(char* buf, const T& t)
{
#if defined(__LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN_BITFIELD) //小端系统
    const char* p = reinterpret_cast<const char*>(&t + 1); //定位到最后一字节的下一字节
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        *(buf++) = *(--p);
    }
#elif defined(__BIG_ENDIAN) || defined(__BIG_ENDIAN_BITFIELD) //大端系统
    memcpy(buf, &t, sizeof(T));
    buf += sizeof(T);
#else
#error "no endian defined"
#endif

    return buf;   
}

}

#endif  // HEADER_NET_BYTE_ORDER_IMPL
