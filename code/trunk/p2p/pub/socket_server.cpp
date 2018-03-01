/*#############################################################################
 * 文件名   : socket_server.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月08日
 * 文件描述 : SocketServer相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "socket_server.hpp"
#include "endpoint.hpp"
#include <glog/logging.h>
#include <boost/bind.hpp>
#include <boost/exception/all.hpp>
#include "socket_connection.hpp"
#include "bc_assert.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: local_address 服务器监听地址
 *         handler 连接创建回调函数
 * 返回值:
 * 修  改:
 *   时间 2013年10月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SocketServer::SocketServer(const EndPoint& local_address, NewConnHandler handler)
	: local_address_(local_address), new_conn_handler_(handler)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SocketServer::~SocketServer()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动服务器
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool SocketServer::Start()
{
	LOG(INFO) << "Start socket server | " << local_address_;

	return Init();
}

/*-----------------------------------------------------------------------------
 * 描  述: 移除连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketServer::RemoveSocketConnection(const ConnectionID& conn_id)
{
	boost::mutex::scoped_lock lock(conn_mutex_);

	ConnectionMapIter iter = conn_map_.find(conn_id);
	if (iter == conn_map_.end())
	{
		LOG(WARNING) << "Can't find socket connection";
		return;
	}
	else
	{
		LOG(INFO) << "Remove one socket connection | " << conn_id;
		conn_map_.erase(iter);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 查找连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SocketConnectionSP SocketServer::FindSocketConnection(const ConnectionID& conn)
{
	boost::mutex::scoped_lock lock(conn_mutex_);

	ConnectionMapIter iter = conn_map_.find(conn);
	if (iter == conn_map_.end())
	{
		return SocketConnectionSP();
	}
	else
	{
		return iter->second.lock();
	}	
}

/*-----------------------------------------------------------------------------
 * 描  述: 新增连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketServer::NewSocketConnection(const SocketConnectionSP& conn)
{
	LOG(INFO) << "Incomming a new socket connection | " << *(conn.get());

	// 将连接插入管理容器
	{
		boost::mutex::scoped_lock lock(conn_mutex_);
		conn_map_.insert(ConnectionMap::value_type(conn->connection_id(), conn));
	}
	
	// 通知新建连接
	if (new_conn_handler_)
	{
		new_conn_handler_(conn);
	}
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: ios io_service
 *         local_address 本地地址
 *         handler 新增连接通知接口
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
TcpSocketServer::TcpSocketServer(io_service& ios, const EndPoint& local_address, NewConnHandler handler)
	: SocketServer(local_address, handler)
	, ios_(ios)
	, acceptor_(ios)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool TcpSocketServer::Init()
{	
	try
	{
		acceptor_.open(boost::asio::ip::tcp::v4());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true)); 
		acceptor_.bind(to_tcp_endpoint(local_address_));
		acceptor_.listen();
	}
	catch (std::exception& e)
	{
		LOG(ERROR) << "Fail to initialize socket server | " << e.what();
		return false;
	}
	
	// 开始接收新的连接
	StartAccept();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 开始执行TCP的Accept
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketServer::StartAccept()
{
	TcpSocketSP accept_socket(new tcp_socket(ios_));
	acceptor_.async_accept(
		*accept_socket,
		boost::bind(&TcpSocketServer::HandleAccept, 
		            shared_from_this(), 
					accept_socket, 
					boost::asio::placeholders::error
		)
	);
}

/*-----------------------------------------------------------------------------
 * 描  述: Accept回调处理
 * 参  数: socket 新产生的socket
 *         ec 错误码
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketServer::HandleAccept(const TcpSocketSP& accept_socket, const error_code& ec)
{
	do 
	{
		if (ec)
		{
			LOG(WARNING) << "Accept error | " << local_address_;
			break;
		}

		TcpSocketConnectionSP conn;
		try
		{
			conn.reset(new TcpSocketConnection(shared_from_this(), 
				accept_socket, to_endpoint(accept_socket->remote_endpoint())));
		}
		catch (std::exception& e)
		{
			LOG(ERROR) << "Fail to create tcp socket connection | " << e.what();
			break;
		}

		NewSocketConnection(conn); // Accept的肯定是新连接

	}while(false);

	StartAccept();	// 继续Accept
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: ios io_service
 *         local_address 本地地址
 *         handler 新增连接通知接口
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
UdpSocketServer::UdpSocketServer(io_service& ios, const EndPoint& local_address, NewConnHandler handler)
	: SocketServer(local_address, handler)
	, buffer_(UDP_MAX_PACKET_LEN)
	, socket_(new udp_socket(ios, to_udp_endpoint(local_address)))
	, is_busy_(false)
	, is_ready_(true)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 对于UDP服务器来说，没有Accept过程，直接在socket接收报文
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool UdpSocketServer::Init()
{
	StartReceive();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 开始接收报文
 * 参  数: ec 错误码
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketServer::StartReceive()
{
	socket_->async_receive_from(
		boost::asio::buffer(buffer_, UDP_MAX_PACKET_LEN),
		sender_endpoint_,
		boost::bind(&UdpSocketServer::HandleReceive, 
		            shared_from_this(), 
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	); 
}

/*-----------------------------------------------------------------------------
 * 描  述: 对于UDP服务器socket，连接的报文接收都由服务器执行，需要考虑性能优化
 * 参  数: ec 错误码
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketServer::HandleReceive(const error_code& ec, uint64 data_len)
{
	if (ec)
	{
		LOG(WARNING) << "Receive error(" <<ec.message() << ") | " 
			         << to_endpoint(sender_endpoint_);
	}
	else
	{
		// 连接处理
		ConnectionID conn(local_address_, to_endpoint(sender_endpoint_), CONN_PROTOCOL_UDP);
		SocketConnectionSP socket_conn = FindSocketConnection(conn);
		if (!socket_conn)
		{
			socket_conn.reset(new UdpSocketConnection(
				shared_from_this(), socket_, to_endpoint(sender_endpoint_)));

			NewSocketConnection(socket_conn);
		}

		// 报文接收处理
		//uint64 available = socket_->available();
		//receive_buffer_.resize(available);
		//socket_->receive(boost::asio::buffer(receive_buffer_));

		//DLOG(INFO) << "Received data size(" << receive_buffer_.size() 
		//	       << ") | " << *(socket_conn.get());

		
		DLOG(INFO) << "Received data size(" << data_len
			       << ") | " << *(socket_conn.get());

		// 回调连接的报文处理函数
		UdpSocketConnectionSP udp_socket_conn = 
			boost::dynamic_pointer_cast<UdpSocketConnection>(socket_conn);
		udp_socket_conn->OnReceiveData(ec, buffer_.data(), data_len);
	}

	// 继续接收报文
	StartReceive();
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送报文
 * 参  数: ec 错误码
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketServer::DoSendData(const SendToBuffer& send_to_buf)
{
	// endpoint后面可以优化
	socket_->async_send_to(
		boost::asio::buffer(send_to_buf.send_buffer.buf + send_to_buf.send_buffer.cursor, 
		                    send_to_buf.send_buffer.len - send_to_buf.send_buffer.cursor), 
		to_udp_endpoint(send_to_buf.remote),
		boost::bind(&UdpSocketServer::HandleSend, 
		            shared_from_this(), 
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * 描  述: 报文发送处理
 * 参  数: ec 错误码
 *         bytes_transferred 发送字节数
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketServer::HandleSend(const error_code& ec, uint64 bytes_transferred)
{
	boost::mutex::scoped_lock lock(send_mutex_);
	SendToBuffer buffer = send_queue_.front();

	// 可以进行下次报文发送
	is_ready_ = true;

	if (!ec)
	{
		buffer.send_buffer.cursor += bytes_transferred;
		BC_ASSERT(buffer.send_buffer.cursor <= buffer.send_buffer.len);

		if (buffer.send_buffer.cursor == buffer.send_buffer.len)
		{
			buffer.connection->OnSendData(ec, buffer.send_buffer.cursor);
			send_queue_.pop();
		}
	}
	else
	{
		buffer.connection->OnSendData(ec, buffer.send_buffer.cursor);
		send_queue_.pop();
	}

	// 释放发送内存
	if (buffer.send_buffer.free_func)
	{
		buffer.send_buffer.free_func(buffer.send_buffer.buf);
	}

	BC_ASSERT(is_busy_);

	// 如果发送队列中还有数据，则继续发送
	if (!send_queue_.empty())
	{
		DoSendData(send_queue_.front());
		is_ready_ = false;
	}
	else
	{
		is_busy_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 报文发送接口
 * 参  数: send_buf 发送数据
 *         bytes_transferred 发送字节数
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketServer::SendData(const SendToBuffer& buf)
{
	boost::mutex::scoped_lock lock(send_mutex_);
	send_queue_.push(buf);

	// 如果队列处于空闲状态，则立即发送数据
	if (!is_busy_ && is_ready_)
	{
		DoSendData(send_queue_.front());
		is_ready_ = false;
		is_busy_ = true;
	}
}

}
