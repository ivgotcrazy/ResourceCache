/**************************************************************************
 *文件名  : connection.hpp
 *创建人  : teck_zhou
 *创建时间: 2013.10.08
 *文件描述: SocketConnection相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 *************************************************************************/

#ifndef HEADER_CONNECTION
#define HEADER_CONNECTION

#include "endpoint.hpp"
#include "bc_typedef.hpp"
#include "hash_stream.hpp"
#include <boost/system/error_code.hpp>

namespace BroadCache
{

/******************************************************************************
 * 描述: 连接错误码，上层协议需要关注的错误码
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
enum ConnectionError
{
	CONN_ERR_SUCCESS = 0,	// 成功
	CONN_ERR_REMOTE_CLOSED,	// 远端关闭
	CONN_ERR_OTHER			// 其他错误
};

inline ConnectionError ToConnErr(const error_code& ec)
{
	switch (ec.value())
	{
	case boost::system::errc::success:
		return CONN_ERR_SUCCESS;
	
	default:
		return CONN_ERR_OTHER;
	}
}

/******************************************************************************
 * 描述: 主动连接和被动连接
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
enum ConnectionType
{
	CONN_INVALID, // 无效连接
	CONN_ACTIVE,  // 主动连接
	CONN_PASSIVE  // 被动连接
};

/******************************************************************************
 * 描述: 连接的协议类型
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
enum ConnectionProtocol
{
	CONN_PROTOCOL_UNKNOWN, // 未知协议连接
	CONN_PROTOCOL_TCP,     // TCP协议连接
	CONN_PROTOCOL_UDP	   // UDP协议连接
};

/******************************************************************************
 * 描述: 唯一标识一个连接的五元组
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
struct ConnectionID
{
	ConnectionID() : protocol(CONN_PROTOCOL_UNKNOWN) {}
	ConnectionID(const EndPoint& local_addr, 
		         const EndPoint& remote_addr, 
				 ConnectionProtocol prot)
		         : local(local_addr)
				 , remote(remote_addr)
				 , protocol(prot) {}

	bool operator==(const ConnectionID& conn_id) const
	{
		return ((protocol == conn_id.protocol) 
			    && (local == conn_id.local)
			    && (remote == conn_id.remote));
	}

	EndPoint local;
	EndPoint remote;
	ConnectionProtocol protocol;
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const ConnectionID& conn_id)
{
	stream << "[" << conn_id.local << "<->" << conn_id.remote << "]";
	return stream;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算hash值，适配unorderd_map的hash函数
 * 参  数: [in] conn_id ConnectionID
 * 返回值: hash值
 * 修  改:
 *   时间 2013年10月9日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
inline std::size_t hash_value(const ConnectionID& conn_id)
{
	HashStream hash_stream;
    hash_stream & conn_id.local & conn_id.remote & conn_id.protocol;

    return hash_stream.value();
}

/******************************************************************************
 * 描述: socket连接使用的报文发送缓冲区
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
struct SendBuffer
{
	SendBuffer() : buf(0), size(0), len(0), cursor(0) {}

	char* buf;     // 发送数据
	uint64 size;   // 内存大小
	uint64 len;    // 发送长度
	uint64 cursor; // 发送游标，多次发送使用

	// 报文发送后回调处理
	typedef boost::function<void(ConnectionError)> SendBufferHandler;
	SendBufferHandler send_buffer_handler;

	// 报文发送后内存释放
	typedef boost::function<void(const char*)> BufferFreeFunc;
	BufferFreeFunc free_func;
};

/******************************************************************************
 * 描述: UDP服务器使用的发送报文缓冲区
 *       由于服务器要向多个客户端发送报文，因此需要包含报文发送的客户端地址；
 *       另外，报文发送完成后需要通知socket连接，因此还需要包含指向连接的指针
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
struct SendToBuffer
{
	SendToBuffer(const UdpSocketConnectionSP& conn, const SendBuffer& buf, const EndPoint& ep) 
		: connection(conn), send_buffer(buf), remote(ep) {}

	UdpSocketConnectionSP connection; // 所属UDP连接
	SendBuffer send_buffer;		      // 发送数据
	EndPoint   remote;                // 发送目的地址
};

}

#endif
