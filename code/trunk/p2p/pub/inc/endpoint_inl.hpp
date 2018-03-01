/*------------------------------------------------------------------------------
 *文件名  : endpoint_inl.hpp
 *创建人  : rosan
 *创建时间: 2013.08.22
 *文件描述: endpoint.hpp中的EndPoint类和全局函数
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_END_POINT_INL
#define HEADER_END_POINT_INL

#include <boost/lexical_cast.hpp>

#include "hash_stream.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: EndPoint类的构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline EndPoint::EndPoint() : ip(0), port(0)
{
}

/*------------------------------------------------------------------------------
 * 描  述: EndPoint类的构造函数
 * 参  数: [in] ep_ip ip地址
 *         [in] ep_port 端口号
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline EndPoint::EndPoint(unsigned long ep_ip, unsigned short ep_port) : ip(ep_ip), port(ep_port)
{
}

/*------------------------------------------------------------------------------
 * 描  述: 判断一个EndPoint对象是否小于另一个EndPoint对象
 * 参  数: [in] lhs 操作符左侧的EndPoint对象
 *         [in] rhs 操作符右侧的EndPoint对象
 * 返回值: 一个EndPoint对象是否小于另一个EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline bool operator<(const EndPoint& lhs, const EndPoint& rhs)
{
    return (lhs.ip < rhs.ip) || (lhs.ip == rhs.ip && lhs.port < rhs.port);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断一个EndPoint对象是否等于另一个EndPoint对象
 * 参  数: [in] lhs 操作符左侧的EndPoint对象
 *         [in] rhs 操作符右侧的EndPoint对象
 * 返回值: 一个EndPoint对象是否等于另一个EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline bool operator==(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs < rhs) && !(rhs < lhs);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断一个EndPoint对象是否大于另一个EndPoint对象
 * 参  数: [in] lhs 操作符左侧的EndPoint对象
 *         [in] rhs 操作符右侧的EndPoint对象
 * 返回值: 一个EndPoint对象是否大于另一个EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline bool operator>(const EndPoint& lhs, const EndPoint& rhs)
{
   return !(lhs < rhs) && (lhs != rhs); 
}

/*------------------------------------------------------------------------------
 * 描  述: 判断一个EndPoint对象是否不等于另一个EndPoint对象
 * 参  数: [in] lhs 操作符左侧的EndPoint对象
 *         [in] rhs 操作符右侧的EndPoint对象
 * 返回值: 一个EndPoint对象是否不等于另一个EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline bool operator!=(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs == rhs);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断一个EndPoint对象是否小于或等于另一个EndPoint对象
 * 参  数: [in] lhs 操作符左侧的EndPoint对象
 *         [in] rhs 操作符右侧的EndPoint对象
 * 返回值: 一个EndPoint对象是否小于或等于另一个EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline bool operator<=(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs > rhs);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断一个EndPoint对象是否大于或等于另一个EndPoint对象
 * 参  数: [in] lhs 操作符左侧的EndPoint对象
 *         [in] rhs 操作符右侧的EndPoint对象
 * 返回值: 一个EndPoint对象是否大于或等于另一个EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline bool operator>=(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs < rhs);
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个整数转换成boost库中的ip地址对象
 * 参  数: [in] ip 代表一个ip地址的整数
 * 返回值: boost库中的ip地址对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline boost::asio::ip::address to_address(unsigned long ip)
{
    // return boost::asio::ip::address(boost::asio::ip::address_v4(ip));
    return boost::asio::ip::address(boost::asio::ip::address_v4(uint32(ip)));
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个点分十进制的ip地址字符串转换成一个整数
 * 参  数: [in] ip 点分十进制的ip地址字符串
 * 返回值: 代表一个ip地址的整数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline unsigned long to_address_ul(const std::string& ip)
{
    return to_address_ul(boost::asio::ip::address::from_string(ip));
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个boost库中的ip地址对象转换成整数
 * 参  数: [in] ip boost库中的ip地址对象
 * 返回值: 代表一个ip地址的整数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline unsigned long to_address_ul(const boost::asio::ip::address& ip)
{
    return ip.to_v4().to_ulong();
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个EndPoint对象转换成udp协议的endpoint对象
 * 参  数: [in] endpoint EndPoint对象
 * 返回值: udp协议的endpoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline boost::asio::ip::udp::endpoint to_udp_endpoint(const EndPoint& endpoint)
{
    return boost::asio::ip::udp::endpoint(to_address(endpoint.ip), endpoint.port);
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个EndPoint对象转换成tcp协议的endpoint对象
 * 参  数: [in] endpoint EndPoint对象
 * 返回值: tcp协议的endpoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline boost::asio::ip::tcp::endpoint to_tcp_endpoint(const EndPoint& endpoint)
{
    return boost::asio::ip::tcp::endpoint(to_address(endpoint.ip), endpoint.port);
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个udp协议的endpoint对象转换成EndPoint对象
 * 参  数: [in] endpoint udp协议的endpoint对象
 * 返回值: EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline EndPoint to_endpoint(const boost::asio::ip::udp::endpoint& endpoint)
{
    return EndPoint(endpoint.address().to_v4().to_ulong(), endpoint.port());
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个tcp协议的endpoint对象转换成EndPoint对象
 * 参  数: [in] endpoint tcp协议的endpoint对象
 * 返回值: EndPoint对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline EndPoint to_endpoint(const boost::asio::ip::tcp::endpoint& endpoint)
{
    return EndPoint(endpoint.address().to_v4().to_ulong(), endpoint.port());
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个整数形式的ip地址转换成一个点分十进制的字符串
 * 参  数: [in] ip 整数形式的ip地址
 * 返回值: 点分十进制的字符串
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline std::string to_string(unsigned long ip)
{
    return boost::asio::ip::address_v4(ip).to_string();
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个EndPoint对象转换成字符串
 *         是点分十进制ip地址加端口号格式的(<ip>:<port>)，如192.168.0.1:8878
 * 参  数: [in] endpoint EndPoint对象
 * 返回值: 转换后的字符串
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline std::string to_string(const EndPoint& endpoint)
{
    return to_string(endpoint.ip) + ':'
        + boost::lexical_cast<std::string>(endpoint.port);
}

/*------------------------------------------------------------------------------
 *描  述: 计算EndPoint对象的hash值
 *参  数: [in] endpoint EndPoint对象
 *返回值: 此EndPoint对象的hash值 
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
inline std::size_t hash_value(const EndPoint& endpoint)
{
    HashStream hash_stream;
    hash_stream & endpoint.ip & endpoint.port;

    return hash_stream.value();
}

/*------------------------------------------------------------------------------
 * 描  述: 将EndPoint对象输出到流中
 * 参  数: [in][out] stream 流对象 
 *         [in] endpoint EndPoint对象
 * 返回值: 输出EndPoint对象后的流对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
inline Stream& operator<<(Stream& stream, const EndPoint& endpoint)
{
    stream << to_string(endpoint);

    return stream;
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个EndPoint对象序列化
 * 参  数: [in][out] serializer 序列化器
 *         [in] ep 被序列化的EndPoint对象
 * 返回值: 序列化EndPoint对象后的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline Serializer& serialize(Serializer& serializer, const EndPoint& ep)
{
    return serializer & ep.ip & ep.port;
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个EndPoint对象反序列化
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] ep 被反序列化的EndPoint对象
 * 返回值: 反序列化EndPoint对象后的反序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline Unserializer& unserialize(Unserializer& unserializer, EndPoint& ep)
{
    return unserializer & ep.ip & ep.port;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取一个EndPoint对象序列化所需缓冲区大小
 * 参  数: [in][out] size_helper 用于获取某个对象序列化所需缓冲区大小
 *         [in] ep EndPoint对象
 * 返回值: 输入的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline SizeHelper& serialize_size(SizeHelper& size_helper, const EndPoint& ep)
{
    return size_helper & ep.ip & ep.port;
}

}

#endif  // HEADER_END_POINT_INL
