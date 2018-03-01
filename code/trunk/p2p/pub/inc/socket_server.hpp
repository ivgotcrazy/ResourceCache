/*#############################################################################
 * 文件名   : socket_server.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月08日
 * 文件描述 : ServerSocket相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef BROADCACHE_SOCKET_SERVER
#define BROADCACHE_SOCKET_SERVER

#include <queue>
#include <set>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "connection.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: socket服务器基类，负责创建和管理socket连接
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
class SocketServer : public boost::noncopyable   
{
public:
	// 连接创建通知接口声明
    typedef boost::function<void(const SocketConnectionSP&)> NewConnHandler;   

	SocketServer(const EndPoint& local_address, NewConnHandler handler);
	virtual ~SocketServer();

    bool Start();
	void RemoveSocketConnection(const ConnectionID& conn_id);

	EndPoint local_address() const { return local_address_; }

protected:
	EndPoint local_address_; // 服务器监听地址

protected:
	SocketConnectionSP FindSocketConnection(const ConnectionID& conn_id);
	void NewSocketConnection(const SocketConnectionSP& conn);

private:
	virtual bool Init() = 0;

private:
	// 此处使用SocketConnection的weak_ptr，外部用户需要负责SocketConnection的声明周期
	typedef boost::unordered_map<ConnectionID, SocketConnectionWP, boost::hash<ConnectionID> > ConnectionMap;
	typedef ConnectionMap::iterator ConnectionMapIter;

	// 管理所有被动连接
    ConnectionMap conn_map_; 
	boost::mutex conn_mutex_;

	NewConnHandler new_conn_handler_;
};

/******************************************************************************
 * 描述: TCP类型的socket服务器
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
class TcpSocketServer : public SocketServer,
	                    public boost::enable_shared_from_this<TcpSocketServer>
{
public:
	TcpSocketServer(io_service& ios, const EndPoint& local_address, NewConnHandler handler);

private:
	virtual bool Init();
	void StartAccept();
	void HandleAccept(const TcpSocketSP& accept_socket, const error_code& ec);

private:
	io_service& ios_;
	boost::asio::ip::tcp::acceptor acceptor_;
};

/******************************************************************************
 * 描述: UDP类型的socket服务器
 * 作者：teck_zhou
 * 时间：2013/10/09
 *****************************************************************************/
class UdpSocketServer : public SocketServer,
	                    public boost::enable_shared_from_this<UdpSocketServer>
{
public:
	UdpSocketServer(io_service& ios, const EndPoint& local_address, NewConnHandler handler);

	void SendData(const SendToBuffer& buf);

private:
	virtual bool Init();
	void StartReceive();
	void HandleReceive(const error_code& ec, uint64 /*bytes_transferred*/);
	void DoSendData(const SendToBuffer& send_to_buf);
	void HandleSend(const error_code& ec, uint64 bytes_transferred);
	
private:
	udp_endpoint sender_endpoint_;
	std::vector<char> receive_buffer_;
	std::vector<char> buffer_;
	UdpSocketSP socket_;

	std::queue<SendToBuffer> send_queue_;
	boost::mutex send_mutex_;
	bool is_busy_;
	bool is_ready_;
};

}

#endif
