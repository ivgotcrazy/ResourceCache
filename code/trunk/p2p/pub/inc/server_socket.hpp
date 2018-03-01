/*------------------------------------------------------------------------------
 *文件名  : server_socket.hpp
 *创建人  : rosan
 *创建时间: 2013.08.22
 *文件描述: ServerSocket TcpServerSocket UdpServerSocket
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_SERVER_SOCKET
#define HEADER_SERVER_SOCKET

#include <vector>

#include <boost/unordered_set.hpp>

#include "endpoint.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: 服务器socket基类
 *作  者: rosan
 *时  间: 2013.08.22
 -----------------------------------------------------------------------------*/ 
class ServerSocket
{
public:
    // 从对端接收到数据后的回调函数
    typedef boost::function<void(const EndPoint&, const error_code&, const char*, uint64)> ReceiveHandler;

    // 数据发送完成后的回调函数
    typedef boost::function<void(const EndPoint&, const error_code&, uint64)> SentHandler;

    ServerSocket(unsigned short local_port, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    ServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    virtual ~ServerSocket();
    
    virtual bool Send(const EndPoint& remote_address, const char* data, uint64 length) = 0;  // 发送数据到对端

    ReceiveHandler receive_handler() const;  // 获取数据接收回调函数
    void set_receive_handler(const ReceiveHandler& handler);  // 设置数据接收回调函数

    SentHandler sent_handler() const;  // 获取数据发送完成回调函数
    void set_sent_handler(const SentHandler& handler);  // 设置数据发送完成回调函数

	EndPoint local_address() const;  // 获取本端地址

	boost::asio::io_service& io_service() const;  // 获取IO服务对象
    
private:
    EndPoint local_address_;  // 本端地址
    boost::asio::io_service& io_service_;  // IO服务对象
    ReceiveHandler receive_handler_;  // 当从对端收到数据后的回调函数
    SentHandler sent_handler_;  // 数据发送完成后的回调函数
};

/*------------------------------------------------------------------------------
 *描  述: TCP服务器socket
 *作  者: rosan
 *时  间: 2013.08.22
 -----------------------------------------------------------------------------*/ 
class TcpServerSocket : public ServerSocket
{
public:
    typedef boost::function<void(const EndPoint&)> ConnectedHandler;  // 对端连接上的回调函数
    typedef boost::function<void(const EndPoint&)> DisconnectedHandler;  // 对端断开连接后的回调函数

    TcpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    TcpServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    virtual ~TcpServerSocket();
    
    virtual bool Send(const EndPoint& remote_address, const char* data, uint64 length) override;  // 发送数据到客户端接口
    
    ConnectedHandler connected_handler() const;  // 获取对端连接上的回调函数
    void set_connected_handler(const ConnectedHandler& handler);  // 设置对端连接上的回调函数

    DisconnectedHandler disconnected_handler() const;  // 获取对端断开连接后的回调函数
    void set_disconnected_handler(const DisconnectedHandler& handler);  // 设置对端断开连接后的回调函数

	boost::unordered_set<EndPoint> GetClients() const;  // 获取对端地址列表

private:
    struct Session;

private:
	void StartAccept();  // 等待来自客户端的连接

    void DoSend(boost::asio::ip::tcp::socket* socket, const EndPoint& remote_address, 
            const char* data, uint64 length);  // 发送数据到对端
    
    bool RemoveSession(const EndPoint& endpoint);  // 删除一个客户端会话（连接）
    
    void HandleAccept(Session* new_session, const boost::system::error_code& error);  // 处理来自客户端的连接

    //接收来自客户端的数据的回调函数
    void HandleRead(const EndPoint& endpoint, const boost::system::error_code& error, uint64 bytes_transferred);

    //当数据发送到客户端后的回调函数
    void HandleWrite(const EndPoint& remote_address, const boost::system::error_code& error, uint64 bytes_transferred);
 
private:
    struct Session  // 客户端会话
    {
        explicit Session(boost::asio::io_service& ios);

        boost::asio::ip::tcp::socket socket;  // 处理客户端连接的socket
        std::vector<char> receive_buffer;  // 接收来自客户端的数据的缓冲区
    }; 

private:
    ConnectedHandler connected_handler_;  // 客户端连接上服务器后的回调函数
    DisconnectedHandler disconnected_handler_;  // 服务器失去连接后的回调函数
    boost::asio::ip::tcp::acceptor acceptor_;  // 客户端连接接收器
    boost::unordered_map<EndPoint, Session*> sessions_;  // 客户端会话
};

/*------------------------------------------------------------------------------
 *描  述: UDP服务器socket
 *作  者: rosan
 *时  间: 2013.08.22
 -----------------------------------------------------------------------------*/ 
class UdpServerSocket : public ServerSocket
{
public:
    UdpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    UdpServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());

    virtual ~UdpServerSocket();
 
    virtual bool Send(const EndPoint& remote_address, const char* data, uint64 length) override;  // 发送数据到对端

private:
    void DoSend(const EndPoint& remote_address, const char* data, uint64 length);  // 发送数据到对端

    // 当收到对端数据后的回调函数
    void HandleReceiveFrom(const boost::system::error_code& error, uint64 bytes_transferred);

    // 当数据发送完成后的回调函数
    void HandleSendTo(const EndPoint& remote_address, const boost::system::error_code& error, 
        uint64 bytes_transferred);

private:
    boost::asio::ip::udp::socket socket_; // socket
    boost::asio::ip::udp::endpoint sender_endpoint_; //最近一个发送数据到UDP服务器的UDP客户端地址
    std::vector<char> receive_buffer_; // 接收数据的缓冲区
};

}

#endif  // HEADER_SERVER_SOCKET
