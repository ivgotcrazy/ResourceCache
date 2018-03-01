/*##############################################################################
 * �ļ���   : net_byte_order_impl_forward.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.10.20
 * �ļ����� : net_byte_order.hpp�Ĺ��������ļ�
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_NET_BYTE_ORDER_IMPL_FORWARD
#define HEADER_NET_BYTE_ORDER_IMPL_FORWARD

#include "single_value_model.hpp"

namespace BroadCache
{

static const size_t kNetSerializeClass = 1;

typedef SingleValueModel<char*, kNetSerializeClass> NetSerializer;  // ��������Ҫת���ֽ�˳�������д�����绺������
typedef SingleValueModel<const char*, kNetSerializeClass> NetUnserializer;  // �����绺�����е�����ת���ɱ�������

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

