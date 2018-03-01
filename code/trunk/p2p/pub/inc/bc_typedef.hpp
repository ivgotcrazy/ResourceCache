/*#############################################################################
 * 文件名   : type_def.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : 宏定义、类型重定义
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BC_TYPEDEF
#define HEADER_BC_TYPEDEF

#include "depend.hpp"

namespace BroadCache
{

#define INTERFACE struct

namespace fs = boost::filesystem;
namespace pb = google::protobuf;

typedef boost::int8_t   int8;
typedef boost::int16_t  int16;
typedef boost::int32_t  int32;
typedef boost::int64_t  int64;

typedef boost::uint8_t  uint8;
typedef boost::uint16_t uint16;
typedef boost::uint32_t uint32;
typedef boost::uint64_t uint64;

typedef boost::int64_t  size_type;

typedef boost::asio::ip::address_v4		 ipv4_address;
typedef boost::asio::ip::address		 ip_address;
typedef boost::posix_time::ptime		 ptime;
typedef boost::posix_time::time_duration time_duration;
typedef boost::system::error_code		 error_code;
typedef boost::asio::io_service			 io_service;
typedef boost::asio::ip::tcp::socket	 tcp_socket;
typedef boost::asio::ip::udp::socket	 udp_socket;
typedef boost::asio::ip::tcp::endpoint	 tcp_endpoint;
typedef boost::asio::ip::udp::endpoint	 udp_endpoint;
typedef boost::asio::deadline_timer		 deadline_timer;

typedef boost::shared_ptr<tcp_socket>	TcpSocketSP;
typedef boost::shared_ptr<udp_socket>	UdpSocketSP;


class BasicConfigParser;
class ClientSocket;
class InfoHash;
class ServerSocket;
class SocketConnection;
class SocketServer;
class TcpSocketConnection;
class TcpClientSocket;
class TcpSocketServer;
class UdpSocketConnection;
class UdpSocketServer;

// shared_ptr封装类型定义, SP: Shared Pointer

typedef boost::shared_ptr<BasicConfigParser>    BasicConfigParserSP;
typedef boost::shared_ptr<ClientSocket>			ClientSocketSP;
typedef boost::shared_ptr<InfoHash>				InfoHashSP;
typedef boost::shared_ptr<ServerSocket>			ServerSocketSP;
typedef boost::shared_ptr<SocketConnection>		SocketConnectionSP;
typedef boost::shared_ptr<SocketServer>		    SocketServerSP;
typedef boost::shared_ptr<TcpClientSocket>		TcpClientSocketSP;
typedef boost::shared_ptr<TcpSocketConnection>	TcpSocketConnectionSP;
typedef boost::shared_ptr<TcpSocketServer>		TcpSocketServerSP;
typedef boost::shared_ptr<UdpSocketConnection>	UdpSocketConnectionSP;
typedef boost::shared_ptr<UdpSocketServer>		UdpSocketServerSP;

// weak_ptr封装类型定义, WP: Weak Pointer

typedef boost::weak_ptr<SocketConnection>	SocketConnectionWP;
typedef boost::weak_ptr<SocketServer>		SocketServerWP;


typedef google::protobuf::Message PbMessage;
typedef boost::shared_ptr<PbMessage> PbMessageSP;

typedef uint32 msg_length_t;

/*******************************************************************************
 *描  述: 消息记录
 *作  者: rosan
 *时  间: 2013.11.11
 ******************************************************************************/
struct MessageEntry
{
    MessageEntry() : msg_data(nullptr), msg_length(0)
    {
    }

    MessageEntry(const char* data, msg_length_t length)
        : msg_data(data), msg_length(length)
    {
    }

    const char* msg_data;  // 消息数据
    msg_length_t msg_length;  // 消息长度
};

typedef std::list<MessageEntry> MessageList;  // 消息记录列表

}

#endif
