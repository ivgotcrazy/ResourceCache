/*#############################################################################
 * 文件名   : client_socket.hpp
 * 创建人   : rosan
 * 创建时间 : 2013年08月22日
 * 文件描述 : ClientSocket TcpClientSocket UdpClientSocket类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_CLIENT_SOCKET
#define HEADER_CLIENT_SOCKET

#include <vector>

#include "depend.hpp"
#include "endpoint.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述：客户端套接字基类
 * 作者：rosan
 * 时间：2013/08/22
 *****************************************************************************/
class ClientSocket
{
public: 
    // 当从服务器收到数据后的回调函数
    typedef boost::function<void(const error_code& error, const char* data, uint64 length)> ReceiveHandler;

    // 当数据已经发送到服务器后的回调函数
    typedef boost::function<void(const error_code& error, uint64 bytes_transferred)> SentHandler;
 
    ClientSocket(boost::asio::io_service& ios, const EndPoint& remote_address,
            const ReceiveHandler& handler = ReceiveHandler());

    virtual ~ClientSocket();

    virtual bool Send(const char* data, uint64 length) = 0;  // 发送数据到对端
    virtual bool Close() = 0;  // 关闭socket
    virtual EndPoint local_address() const = 0;  // 获取本端地址
    
	ReceiveHandler receive_handler() const;  // 获取接收数据回调函数
	void set_receive_handler(const ReceiveHandler& handler);  // 设置接收数据回调函数

	SentHandler sent_handler() const;  // 获取数据发送完成回调函数
	void set_sent_handler(const SentHandler& handler);  // 设置数据发送完成回调函数

	boost::asio::io_service& io_service() const;  // 获取IO服务对象

    EndPoint remote_address() const;  // 获取对端地址

private:
    EndPoint remote_address_; // 服务器地址
    ReceiveHandler receive_handler_; // 从对端收到数据后的回调函数
    SentHandler sent_handler_; // 当数据发送完成后的回调函数
    boost::asio::io_service& io_service_; // IO服务对象
};

/******************************************************************************
 * 描述：TCP客户端socket
 * 作者：rosan
 * 时间：2013/08/22
 *****************************************************************************/
class TcpClientSocket : public ClientSocket
{
public:
    TcpClientSocket(boost::asio::io_service& ios, const EndPoint& remote_address,
        const ReceiveHandler& handler = ReceiveHandler());

    virtual ~TcpClientSocket();
    
    virtual bool Send(const char* data, uint64 length) override;  // 发送数据到对端
    virtual bool Close() override;  // 关闭socket
    virtual EndPoint local_address() const override;  // 获取本端地址

    bool IsConnected() const;  // 是否连上对端

private:
    void Connect();  // 连接对端地址
    void DoSend(const char* data, uint64 length);  // 向对端发送数据
    void HandleConnect(const boost::system::error_code& error);  // 连上对端后的回调函数
    void HandleRead(const boost::system::error_code& error, uint64 bytes_transferred);  // 当从对端收到数据后的回调函数
    void HandleSent(const boost::system::error_code& error, uint64 bytes_transferred);  // 当数据发送完成后的回调函数

private:
    bool connected_; //标识对端是否连接上
    std::vector<char> receive_buffer_; //接收数据缓冲区
    boost::asio::ip::tcp::socket socket_; //socket对象
};

/******************************************************************************
 * 描述：UDP客户端socket
 * 作者：rosan
 * 时间：2013/08/22
 *****************************************************************************/
class UdpClientSocket : public ClientSocket
{
public:
    UdpClientSocket(boost::asio::io_service& ios, const EndPoint& remote_address,
        const ReceiveHandler& handler = ReceiveHandler());

    virtual ~UdpClientSocket();
 
    virtual bool Send(const char* data, uint64 length) override;  // 发送数据到服务器
    virtual bool Close() override;  // 关闭socket
    virtual EndPoint local_address() const override;   // 获取本地地址

private:
    void DoSend(const char* data, uint64 length);  // 发送数据到对端
    void HandleReceiveFrom(const boost::system::error_code& error, uint64 bytes_transferred);  // 从服务器接收到数据后的回调函数
    void HandleSendTo(const boost::system::error_code& error, uint64 bytes_transferred);  // 数据发送完成后的回调函数

private:
    std::vector<char> receive_buffer_; //接收数据缓冲区
    boost::asio::ip::udp::socket socket_; //socket对象
    boost::asio::ip::udp::endpoint endpoint_; //对端地址
};

}

#endif  // HEADER_CLIENT_SOCKET
