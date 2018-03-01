/*##############################################################################
 * 文件名   : net_byte_order_impl_forward.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.10.20
 * 文件描述 : net_byte_order.hpp的功能声明文件
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_NET_BYTE_ORDER_IMPL_FORWARD
#define HEADER_NET_BYTE_ORDER_IMPL_FORWARD

#include "single_value_model.hpp"

namespace BroadCache
{

static const size_t kNetSerializeClass = 1;

typedef SingleValueModel<char*, kNetSerializeClass> NetSerializer;  // 将本地需要转换字节顺序的数据写到网络缓冲区中
typedef SingleValueModel<const char*, kNetSerializeClass> NetUnserializer;  // 将网络缓冲区中的数据转换成本地数据

template<typename T>
inline T NetToHost(const T& t);

template<typename T>
inline T NetToHost(const char* buf);

template<typename T>
inline const char* NetToHost(const char* buf, T& t);

template<typename T>
inline T HostToNet(const T& t);

template<typename T>
inline char* HostToNet(char* buf, const T& t);

template<typename T>
inline NetSerializer& operator&(NetSerializer& serializer, const T& t);

template<typename T>
inline NetUnserializer& operator&(NetUnserializer& unserializer, T& t);

}

#endif  // HEADER_NET_BYTE_ORDER_IMPL_FORWARD

