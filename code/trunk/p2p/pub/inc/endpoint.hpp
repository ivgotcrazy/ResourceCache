/*------------------------------------------------------------------------------
 *文件名  : endpoint.hpp
 *创建人  : rosan
 *创建时间: 2013.08.22
 *文件描述: EndPoint类，以及其和boost endpoint的相互转换的函数
            定义了EndPoint相关的运算符
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_END_POINT
#define HEADER_END_POINT

#include "depend.hpp"
#include "serialize.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: 网络地址，包括ip地址和端口号
 *作  者: rosan
 *时  间: 2013.08.22
 -----------------------------------------------------------------------------*/ 
struct EndPoint
{
    EndPoint();
    EndPoint(unsigned long ep_ip, unsigned short ep_port);

	unsigned long ip;  // ip地址
	unsigned short	port;  // 端口号
};

// EndPoint重载比较操作符
inline bool operator<(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator==(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator>(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator!=(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator<=(const EndPoint& lhs, const EndPoint& rhs);
inline bool operator>=(const EndPoint& lhs, const EndPoint& rhs);

// ip地址转换函数,以unsigned long为核心
inline boost::asio::ip::address to_address(unsigned long ip);
inline unsigned long to_address_ul(const std::string& ip);
inline unsigned long to_address_ul(const boost::asio::ip::address& ip);
inline boost::asio::ip::udp::endpoint to_udp_endpoint(const EndPoint& endpoint);
inline boost::asio::ip::tcp::endpoint to_tcp_endpoint(const EndPoint& endpoint);
inline EndPoint to_endpoint(const boost::asio::ip::udp::endpoint& endpoint);
inline EndPoint to_endpoint(const boost::asio::ip::tcp::endpoint& endpoint);
inline std::string to_string(unsigned long ip);
inline std::string to_string(const EndPoint& endpoint);

inline std::size_t hash_value(const EndPoint& endpoint);  // 计算EndPoint对象的hash值

template<class Stream>
inline Stream& operator<<(Stream& stream, const EndPoint& endpoint);  // 将EndPoint输出到流中

// 实现EndPoint对象的序列化,反序列化操作
inline Serializer& serialize(Serializer& serializer, const EndPoint& ep);
inline Unserializer& unserialize(Unserializer& unserializer, EndPoint& ep);
inline SizeHelper& serialize_size(SizeHelper& size_helper, const EndPoint& ep);

}

#include "endpoint_inl.hpp"

#endif  // HEADER_END_POINT
