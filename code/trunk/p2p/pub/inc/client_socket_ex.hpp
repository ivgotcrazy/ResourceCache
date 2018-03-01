/*##############################################################################
 * 文件名   : client_socket_ex.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.28
 * 文件描述 : TcpClientSocketEx类的声明文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_CLIENT_SOCKET_EX
#define HEADER_CLIENT_SOCKET_EX

#include <vector>

#include "endpoint.hpp"
#include "bc_typedef.hpp"
#include "timer.hpp"

namespace BroadCache
{

/*******************************************************************************
 * 描  述: 此类实现TCP客户端断线重连功能
 * 作  者: rosan
 * 时  间: 2013.11.28
 ******************************************************************************/
class TcpClientSocketEx
{
public: 
    // 当从服务器收到数据后的回调函数
    typedef boost::function<void(const error_code& error, const char* data, uint64 length)> ReceiveHandler;

    // 当数据已经发送到服务器后的回调函数
    typedef boost::function<void(const error_code& error, uint64 bytes_transferred)> SentHandler;

	// 当对端连接上后的回调函数
	typedef boost::function<void()> ConnectedHandler;

	// 当对端失去连接后的回调函数
	typedef boost::function<void()> DisconnectedHandler;
 
    TcpClientSocketEx(boost::asio::io_service& ios, const EndPoint& remote_address);
    ~TcpClientSocketEx();

    bool Send(const char* data, uint64 length);  // 发送数据到对端
    void Close();  // 关闭socket

    EndPoint local_address() const;  // 获取本端地址
	EndPoint remote_address() const;  // 获取对端地址
    
	ReceiveHandler receive_handler() const;  // 获取接收数据回调函数
	void set_receive_handler(const ReceiveHandler& handler);  // 设置接收数据回调函数

	SentHandler sent_handler() const;  // 获取数据发送完成回调函数
	void set_sent_handler(const SentHandler& handler);  // 设置数据发送完成回调函数

	ConnectedHandler connected_handler() const;
	void set_connected_handler(const ConnectedHandler& handler);

	DisconnectedHandler disconnected_handler() const;
	void set_disconnected_handler(const DisconnectedHandler& handler);

	boost::asio::io_service& io_service() const;  // 获取IO服务对象

	bool IsConnected() const;  // 是否连上对端

private:
	void Connect();  // 连接对端地址
	void DoSend(const char* data, uint64 length);  // 向对端发送数据
	void HandleConnect(const boost::system::error_code& error);  // 连上对端后的回调函数
	void HandleRead(const boost::system::error_code& error, uint64 bytes_transferred);  // 当从对端收到数据后的回调函数
	void HandleSent(const boost::system::error_code& error, uint64 bytes_transferred);  // 当数据发送完成后的回调函数
	void OnTimer();  // 断线重连的定时器

private:
	boost::asio::io_service& io_service_; // IO服务对象
    EndPoint remote_address_; // 服务器地址
	boost::asio::ip::tcp::socket socket_; //socket对象
    ReceiveHandler receive_handler_; // 从对端收到数据后的回调函数
    SentHandler sent_handler_; // 当数据发送完成后的回调函数
	ConnectedHandler connected_handler_;
	DisconnectedHandler disconnected_handler_;
	bool connected_; //标识对端是否连接上
	std::vector<char> receive_buffer_; //接收数据缓冲区
	Timer timer_;  // 断线重连的定时器
};

}  // namespace BroadCache

#endif  // HEADER_CLIENT_SOCKET_EX
