/*------------------------------------------------------------------------------
 *�ļ���  : endpoint.hpp
 *������  : rosan
 *����ʱ��: 2013.08.22
 *�ļ�����: EndPoint�࣬�Լ����boost endpoint���໥ת���ĺ���
            ������EndPoint��ص������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_END_POINT
#define HEADER_END_POINT

#include "depend.hpp"
#include "serialize.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *��  ��: �����ַ������ip��ַ�Ͷ˿ں�
 *��  ��: rosan
 *ʱ  ��: 2013.08.22
 -----------------------------------------------------------------------------*/ 
struct EndPoint
{
    EndPoint();
    EndPoint(unsigned long ep_ip, unsigned short ep_port);

	unsigned long ip;  // ip��ַ
	unsigned short	port;  // �˿ں�
};

// EndPoint���رȽϲ�����
inline bool operator<(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator==(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator>(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator!=(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator<=(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator>=(const EndPoint& lhs, const EndPoint& rhs);

// ip��ַת������,��unsigned longΪ����
inline boost::asio::ip::address to_address(unsigned long ip);
inline unsigned long to_address_ul(const std::string& ip);
inline unsigned long to_address_ul(const boost::asio::ip::address& ip);
inline boost::asio::ip::udp::endpoint to_udp_endpoint(const EndPoint& endpoint);
inline boost::asio::ip::tcp::endpoint to_tcp_endpoint(const EndPoint& endpoint);
inline EndPoint to_endpoint(const boost::asio::ip::udp::endpoint& endpoint);
inline EndPoint to_endpoint(const boost::asio::ip::tcp::endpoint& endpoint);
inline std::string to_string(unsigned long ip);
inline std::string to_string(const EndPoint& endpoint);

inline std::size_t hash_value(const EndPoint& endpoint);  // ����EndPoint�����hashֵ

template<class Stream>
inline Stream& operator<<(Stream& stream, const EndPoint& endpoint);  // ��EndPoint���������

// ʵ��EndPoint��������л�,�����л�����
inline Serializer& serialize(Serializer& serializer, const EndPoint& ep);
inline Unserializer& unserialize(Unserializer& unserializer, EndPoint& ep);
inline SizeHelper& serialize_size(SizeHelper& size_helper, const EndPoint& ep);

}

#include "endpoint_inl.hpp"

#endif  // HEADER_END_POINT
