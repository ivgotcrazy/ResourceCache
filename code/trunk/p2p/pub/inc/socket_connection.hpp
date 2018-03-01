/**************************************************************************
 *文件名  : socket_connection.hpp
 *创建人  : teck_zhou
 *创建时间: 2013.10.08
 *文件描述: SocketConnection相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 *************************************************************************/

#ifndef BROADCACHE_SOCKET_CONNECTION
#define BROADCACHE_SOCKET_CONNECTION

#include <atomic>
#include <queue>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "connection.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: socket连接基类，负责报文的收发
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
class SocketConnection : public boost::noncopyable, 
	                     public boost::enable_shared_from_this<SocketConnection>
{
public:
	typedef boost::function<void(ConnectionError, const char*, uint64)> ReceiveHandler;
	typedef boost::function<void(ConnectionError, uint64)> SendHandler;
	typedef boost::function<void(ConnectionError)> ConnectHandler;

	SocketConnection(const EndPoint& remote); // 主动连接
	SocketConnection(const SocketServerSP& server, const EndPoint& remote); // 被动连接
	virtual ~SocketConnection();

	// 主动连接需要调用此接口显示进行连接，当前暂不支持重复连接
	void Connect();
	void SendData(const SendBuffer& send_buf);
	void Close();
	
	void set_connect_handler(ConnectHandler handler) { connect_handler_ = handler; }
	void set_recv_handler(ReceiveHandler handler) { recv_handler_ = handler; }
	void set_send_handler(SendHandler hanlder) { send_handler_ = hanlder; }

	bool is_connected() const { return is_connected_; }
	ConnectionID connection_id() const { return conn_id_; }
	ConnectionType connection_type() const { return conn_type_; }

	bool operator==(const SocketConnection& conn) const { return conn.conn_id_ == conn_id_; }

protected:
	void StartSendWithLock();
	void OnConnect(const error_code& ec);
	void OnReceive(const error_code& ec, const char* buf, uint64 bytes_transferred);
	void OnSend(const error_code& ec, uint64 bytes_transferred);

protected:
	ConnectionID conn_id_;
	ConnectionType conn_type_;

private:
	virtual void DoConnect() = 0;
	virtual void DoSendData(const SendBuffer& send_buf) = 0;
	virtual void DoClose() = 0;

private:
	// 连接所属socket服务器，主动连接无意义
	SocketServerWP socket_server_; 

	ConnectHandler connect_handler_;
	ReceiveHandler recv_handler_;
	SendHandler send_handler_;

	std::queue<SendBuffer> send_buffer_queue_;
	boost::mutex send_queue_mutex_;
	
	// 连接是否已经建立，此标志用来控制是否可以进行报文收发
	bool is_connected_;

	// 前一个报文是否已经发送完毕并收到相应异步响应(true:是，false:否)
	// 此标志用来控制数序逐个进行报文发送
	bool is_ready_;
};

/******************************************************************************
 * 描述: TCP类型的socket连接
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
class TcpSocketConnection : public SocketConnection
{
public:
	TcpSocketConnection(io_service& ios, const EndPoint& remote); // 同步主动连接
    TcpSocketConnection(io_service& ios, const EndPoint& remote, ConnectHandler handler); // 异步主动连接
	TcpSocketConnection(const TcpSocketServerSP& server, const TcpSocketSP& socket, const EndPoint& remote); // 被动连接
	~TcpSocketConnection();

private:
	virtual void DoConnect();
	virtual void DoSendData(const SendBuffer& send_buf);
	virtual void DoClose();

private:
	void StartReceive();
	void HandleConnect(const error_code& ec);
	void HandleReceive(const error_code& ec, uint64 /*bytes_transferred*/);
	void HandleSend(const error_code& ec, uint64 bytes_transferred);
	void CloseSocket();

private:
	TcpSocketSP socket_;
	ConnectHandler connect_handler_;
	std::vector<char> receive_buffer_;
};

/******************************************************************************
 * 描述: UDP类型的socket连接
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
class UdpSocketConnection : public SocketConnection
{
public:
	UdpSocketConnection(io_service& ios, const EndPoint& remote); // 主动连接
	UdpSocketConnection(const UdpSocketServerSP& server, const UdpSocketSP& socket, const EndPoint& remote); // 被动连接
	~UdpSocketConnection();

	// UDP被动连接的报文收发由服务器socket负责
	void OnReceiveData(const error_code& ec, const char* buf, uint64 len);
	void OnSendData(const error_code& ec, uint64 bytes_transferred);

private:
	virtual void DoConnect();
	virtual void DoSendData(const SendBuffer& send_buf);
	virtual void DoClose();

private:
	void StartReceive();
	void HandleReceive(const error_code& ec, uint64 /*bytes_transferred*/);
	void HandleSend(const error_code& ec, uint64 bytes_transferred);
	void CloseSocket();

private:
	UdpSocketSP socket_;
	udp_endpoint sender_endpoint_;
	std::vector<char> receive_buffer_;
	UdpSocketServerSP udp_server_;
};

/*-----------------------------------------------------------------------------
 * 描  述: 打印连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Stream>
inline Stream& operator<<(Stream& stream, const SocketConnection& conn)
{
	stream << conn.connection_id();
	return stream;
}

}

#endif
