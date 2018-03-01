/*#############################################################################
 * 文件名   : socket_connection.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月08日
 * 文件描述 : SocketConnection相关类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "socket_connection.hpp"
#include "socket_server.hpp"
#include <glog/logging.h>
#include <boost/bind.hpp>
#include "bc_assert.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: remote 远端地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SocketConnection::SocketConnection(const EndPoint& remote)
	: is_connected_(false), is_ready_(true)
{
	conn_id_.remote = remote;
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: server 所属socket服务器
 *         remote 远端地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SocketConnection::SocketConnection(const SocketServerSP& server, const EndPoint& remote)
	: socket_server_(server), is_connected_(false), is_ready_(true)
{
	conn_id_.remote = remote;
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: server 所属socket服务器
 *         remote 远端地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SocketConnection::~SocketConnection()
{
	LOG(INFO) << ">>> Destructing socket connection | " << conn_id_;

	is_connected_ = false;

	// 释放未来得及发送的数据内存，但不会通知用户
	boost::mutex::scoped_lock lock(send_queue_mutex_);
	while (!send_buffer_queue_.empty())
	{
		SendBuffer& send_buf = send_buffer_queue_.front();
		if (send_buf.free_func)
		{
			send_buf.free_func(send_buf.buf);
		}
		send_buffer_queue_.pop();
	}

	// 如果是被动连接则通知服务器删除对应连接
	if (conn_type_ == CONN_PASSIVE)
	{
		SocketServerSP server = socket_server_.lock();
		if (server)
		{
			server->RemoveSocketConnection(conn_id_);
		}
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketConnection::Connect()
{
	// 调用子类方法执行连接动作
	DoConnect();
}

/*-----------------------------------------------------------------------------
 * 描  述: 关闭连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketConnection::Close()
{
	LOG(INFO) << "### Close socket connection | " << connection_id();

	DoClose(); // 调用子类方法执行关闭动作
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送数据
 * 参  数: [in] send_buf 数据
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketConnection::SendData(const SendBuffer& send_buf)
{
	BC_ASSERT(send_buf.cursor < send_buf.len);

	// 对于UDP被动连接，报文直接扔给给服务器，不使用连接的缓存
	if ((conn_type_ == CONN_PASSIVE) && (conn_id_.protocol == CONN_PROTOCOL_UDP))
	{
		DoSendData(send_buf);
		return;
	}

	// 先将待发送的buffer放入队列
	boost::mutex::scoped_lock lock(send_queue_mutex_);
	send_buffer_queue_.push(send_buf);
	
	// 如果连接已经建立并且处于空闲状态，则马上发送报文，
	// 否则需要等到连接建立成功或者前面的buffer发送完后再继续发送报文
	if (is_connected_ && is_ready_)
	{
		DoSendData(send_buffer_queue_.front());
		is_ready_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送数据
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketConnection::StartSendWithLock()
{
	boost::mutex::scoped_lock lock(send_queue_mutex_);

	if (!send_buffer_queue_.empty() && is_ready_)
	{
		DoSendData(send_buffer_queue_.front());
		is_ready_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 关闭连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketConnection::OnConnect(const error_code& ec)
{
	is_connected_ = true;

	if (connect_handler_)
	{
		connect_handler_(ToConnErr(ec));
	}

	// 开始发送报文
	StartSendWithLock();
}

/*-----------------------------------------------------------------------------
 * 描  述: 上层接收处理
 * 参  数: [in] ec 错误码
 *         [in] buf 接收数据
 *         [in] bytes_transferred 接收数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketConnection::OnReceive(const error_code& ec, const char* buf, uint64 bytes_transferred)
{
	if (ec)
	{
		LOG(WARNING) << "Connection error(" << ec.message() << ") | " << *this;
	}

	ConnectionError error = ToConnErr(ec);

	// 对于接收报文长度为0需要特殊处理
	if (bytes_transferred == 0)
	{
		if (conn_id_.protocol == CONN_PROTOCOL_TCP)
		{
			LOG(INFO) << "Peer closed connection | " << *this;
			error = CONN_ERR_REMOTE_CLOSED;
		}
	}

	// 回调用户报文处理函数
	if (recv_handler_)
	{
		recv_handler_(error, buf, bytes_transferred);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 上层发送处理
 * 参  数: [in] ec 错误码
 *         [in] bytes_transferred 发送字节数
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SocketConnection::OnSend(const error_code& ec, uint64 bytes_transferred)
{
	// 对于UDP被动连接，报文发送由socket服务器负责，此处将结果直接转发给用户
	if ((conn_type_ == CONN_PASSIVE) && (conn_id_.protocol == CONN_PROTOCOL_UDP))
	{
		if(send_handler_)
		{
			send_handler_(ToConnErr(ec), bytes_transferred);
		}
		return;
	}

	boost::mutex::scoped_lock lock(send_queue_mutex_);

	is_ready_ = true; // 可以进行下一次报文发送

	// 队列中已无报文收到发送响应，这种情况先记录下来
	if (send_buffer_queue_.empty())
	{
		LOG(WARNING) << "None send buffer while received send callback";
		return;
	}

	SendBuffer& buffer = send_buffer_queue_.front();
	if (!ec) // 发送成功
	{
		buffer.cursor += bytes_transferred;
		
		if (buffer.cursor > buffer.len)
		{
			LOG(ERROR) << "*****************************************************";
			LOG(ERROR) << "cursor: " << buffer.cursor << " len: " << buffer.len;
			LOG(ERROR) << "*****************************************************";
		}

		BC_ASSERT(buffer.cursor <= buffer.len);
	}

	// 发送失败或者一个buffer发送完成
	if (ec || buffer.cursor == buffer.len)
	{
		if (buffer.send_buffer_handler) // 回调用户处理
		{
			buffer.send_buffer_handler(ToConnErr(ec));
		}

		if (buffer.free_func) // 释放内存
		{
			buffer.free_func(buffer.buf);
		}

		send_buffer_queue_.pop();
	}

	// 队列中还有数据则继续发送
	if (!send_buffer_queue_.empty())
	{
		SendBuffer& next_buffer = send_buffer_queue_.front();
		DoSendData(next_buffer);

		is_ready_ = false;
	}
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: TCP主动连接构造函数
 * 参  数: ios io_service
 *         remote 远端地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
TcpSocketConnection::TcpSocketConnection(io_service& ios, const EndPoint& remote)
	: SocketConnection(remote)
	, socket_(new tcp_socket(ios, tcp_endpoint(boost::asio::ip::tcp::v4(), 0)))
{
	conn_type_ = CONN_ACTIVE;
	conn_id_.protocol = CONN_PROTOCOL_TCP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * 描  述: TCP被动连接构造函数
 * 参  数: server 所属socket服务器
 *         socket 连接socket
 *         remote 远端地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
TcpSocketConnection::TcpSocketConnection(const TcpSocketServerSP& server, 
	const TcpSocketSP& socket, const EndPoint& remote)
	: SocketConnection(server, remote)
	, socket_(socket)
{
	conn_type_ = CONN_PASSIVE;
	conn_id_.protocol = CONN_PROTOCOL_TCP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
TcpSocketConnection::~TcpSocketConnection()
{
	LOG(INFO) << ">>> Destructing TcpSocketConnection | " << connection_id();  

	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * 描  述: 连接处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::DoConnect()
{
	if (conn_type_ == CONN_PASSIVE)
	{
		// TCP被动连接不需要再去connect了，直接通知上层
		OnConnect(boost::system::errc::make_error_code(boost::system::errc::success));

		// 开始接收数据
		StartReceive();
	}
	else
	{
		socket_->async_connect(
			to_tcp_endpoint(conn_id_.remote), 
			boost::bind(&TcpSocketConnection::HandleConnect, 
			boost::dynamic_pointer_cast<TcpSocketConnection>(shared_from_this()),
			boost::asio::placeholders::error
			)
		);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送报文
 * 参  数: [in] send_buf 报文
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::DoSendData(const SendBuffer& send_buf)
{
	boost::asio::async_write(
		*socket_, 
		boost::asio::buffer(send_buf.buf + send_buf.cursor, send_buf.len - send_buf.cursor), 
		boost::bind(&TcpSocketConnection::HandleSend, 
					boost::dynamic_pointer_cast<TcpSocketConnection>(shared_from_this()), 
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试去使能socket上的读/写操作，并关闭socket
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::DoClose()
{
	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * 描  述: 由于CloseSocket是纯虚函数，需要另外提供一个函数供析构函数调用
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::CloseSocket()
{
	error_code ec;
	socket_->close(ec);
	if (ec)
	{
		LOG(WARNING) << "Fail to close socket | " << ec.message();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 主动连接建立成功后，马上进入接收报文状态
 * 参  数: [in] ec 错误码
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::HandleConnect(const error_code& ec)
{
	OnConnect(ec);

	//TODO: 后续可以考虑优化为尝试多次连接
	if (ec)
	{
		LOG(WARNING) << "Failed to connect | " << *this;
	}
	else
	{
		LOG(INFO) << "Success to connect | " << *this;	
	}

	// 开始接收数据
	StartReceive();
}

/*-----------------------------------------------------------------------------
 * 描  述: 进入接收报文状态
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::StartReceive()
{
	// 此接口读到数据就会返回
	socket_->async_read_some(
		boost::asio::null_buffers(), 
		boost::bind(&TcpSocketConnection::HandleReceive, 
		            boost::dynamic_pointer_cast<TcpSocketConnection>(shared_from_this()), 
					boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * 描  述: 报文接收处理
 * 参  数: [in] ec 错误码
 *         [in] uint64 接收报文长度，未使用
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::HandleReceive(const error_code& ec, uint64)
{
	if (ec)
	{
		LOG(WARNING) << "Receive data error(" << ec << ") | " << *this;
		OnReceive(ec, 0, 0);
		return;
	}

	uint64 available = socket_->available();
	receive_buffer_.resize(available);
	socket_->read_some(boost::asio::buffer(receive_buffer_));

	// 通知上层
	OnReceive(ec, receive_buffer_.data(), receive_buffer_.size());

	// 对于TCP来说，接收0字节认为连接已经关闭，不应该再继续接收
	if (receive_buffer_.size() != 0)
	{
		StartReceive();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 报文发送处理
 * 参  数: [in] ec 错误码
 *         [in] bytes_transferred 发送报文长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::HandleSend(const error_code& ec, uint64 bytes_transferred)
{
	// 通知上层
	OnSend(ec, bytes_transferred);
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: UDP主动连接构造函数
 * 参  数: ios io_service
 *         remote 远端地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
UdpSocketConnection::UdpSocketConnection(io_service& ios, const EndPoint& remote)
	: SocketConnection(remote)
	, socket_(new udp_socket(ios, udp_endpoint(boost::asio::ip::udp::v4(), 0)))
{
	conn_type_ = CONN_ACTIVE;
	conn_id_.protocol = CONN_PROTOCOL_UDP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * 描  述: UDP被动连接构造函数
 * 参  数: [in] server 所属socket服务器
 *         [in] socket 连接socket
 *         [in] remote 远端地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
UdpSocketConnection::UdpSocketConnection(const UdpSocketServerSP& server, 
	const UdpSocketSP& socket, const EndPoint& remote)
	: SocketConnection(server, remote)
	, socket_(socket)
	, udp_server_(server)
{
	conn_type_ = CONN_PASSIVE;
	conn_id_.protocol = CONN_PROTOCOL_UDP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
UdpSocketConnection::~UdpSocketConnection()
{
	LOG(INFO) << ">>> Destructing UdpSocketConnection | " << connection_id();

	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * 描  述: 连接处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月02日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::DoConnect()
{
	// 对于UDP来说，没有连接过程，创建后即认为已经连接OK
	OnConnect(boost::system::errc::make_error_code(boost::system::errc::success));

	// 对于主动连接，立即开始接收报文
	if (conn_type_ == CONN_ACTIVE)
	{
		StartReceive();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送数据处理
 * 参  数: [in] send_buf 报文
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::DoSendData(const SendBuffer& send_buf)
{
	if (conn_type_ == CONN_PASSIVE)
	{
		SendToBuffer buffer(
			boost::dynamic_pointer_cast<UdpSocketConnection>(shared_from_this()), 
			send_buf, 
			conn_id_.remote
		);
		udp_server_->SendData(buffer);
	}
	else
	{
		// endpoint后面可以优化
		socket_->async_send_to(
			boost::asio::buffer(send_buf.buf + send_buf.cursor, send_buf.len - send_buf.cursor), 
			to_udp_endpoint(conn_id_.remote),
			boost::bind(&UdpSocketConnection::HandleSend, 
			            boost::dynamic_pointer_cast<UdpSocketConnection>(shared_from_this()), 
			            boost::asio::placeholders::error,
			            boost::asio::placeholders::bytes_transferred
			)
		);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试去使能socket上的读/写操作，并关闭socket
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::DoClose()
{
	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * 描  述: 由于CloseSocket是纯虚函数，需要另外提供一个函数供析构函数调用
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::CloseSocket()
{
	if (connection_id().protocol == CONN_PROTOCOL_UDP && connection_type() == CONN_PASSIVE)
	{
		return;
	}
	
	error_code ec;
	socket_->close(ec);
	if (ec)
	{
		LOG(WARNING) << "Fail to close socket | " << ec.message();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 接收报文
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::StartReceive()
{
	//从服务器接收数据
	socket_->async_receive_from(
		boost::asio::null_buffers(),
		sender_endpoint_, 
		boost::bind(&UdpSocketConnection::HandleReceive, 
		            boost::dynamic_pointer_cast<UdpSocketConnection>(shared_from_this()),
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * 描  述: UDP主动连接报文接收处理函数
 * 参  数: [in] ec 错误码
 *         [in] len
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::HandleReceive(const error_code& ec, uint64 len)
{
	if (!ec)
	{
		// 报文接收处理
		uint64 available = socket_->available();
		receive_buffer_.resize(available);
		socket_->receive(boost::asio::buffer(receive_buffer_));
	}

	// 通知上层
	OnReceive(ec, receive_buffer_.data(), receive_buffer_.size());
	
	// 继续接收
	if (!ec)
	{
		StartReceive();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: UDP主动连接处理报文发送
 * 参  数: [in] ec 错误码
 *         [in] bytes_transferred 报文发送大小
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::HandleSend(const error_code& ec, uint64 bytes_transferred)
{
	// 通知上层
	OnSendData(ec, bytes_transferred);
}

/*-----------------------------------------------------------------------------
 * 描  述: UDP被动连接，报文由服务器分发过来，这里只做一个转发
 * 参  数: [in] ec 错误码
 *         [in] buf 接收数据
 *         [in] len 接收数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   枋?创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::OnReceiveData(const error_code& ec, const char* buf, uint64 len)
{
	// 通知上层
	OnReceive(ec, buf, len);
}

/*-----------------------------------------------------------------------------
 * 描  述: UDP被动连接，报文发送结果由服务器通知过来，这里只做一个转发
 * 参  数: [in] ec 错误码
 *         [in] bytes_transferred 接收数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::OnSendData(const error_code& ec, uint64 bytes_transferred)
{
	// 通知上层
	OnSend(ec, bytes_transferred);
}

}
