/*##############################################################################
 * 文件名   : client_socket_ex.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.28
 * 文件描述 : TcpClientSocketEx的实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "client_socket_ex.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: TcpClientSocketEx构造函数
 *参  数: [in]ios IO服务
        : [in]remote_address 对端地址
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::TcpClientSocketEx(boost::asio::io_service& ios,
    const EndPoint& remote_address) : 
	io_service_(ios),
	remote_address_(remote_address),
	socket_(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0)),
    connected_(false),
	timer_(ios, boost::bind(&TcpClientSocketEx::OnTimer, this), 1)
{
    timer_.Start();
}

/*------------------------------------------------------------------------------
 *描  述: TcpClientSocketEx的析构函数
 *参  数:
 *返回值:
 *修  改:
 *  时间 2013.08.20 
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::~TcpClientSocketEx()
{
	Close();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取socket对端的地址
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
EndPoint TcpClientSocketEx::remote_address() const
{
    return remote_address_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数
 *         此回调函数是接收对端数据后的处理函数
 * 参  数:
 * 返回值: 接收对端数据后的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::ReceiveHandler TcpClientSocketEx::receive_handler() const
{
    return receive_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         此回调函数是接收对端数据后的处理函数
 * 参  数: [in]handler 接收对端数据后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_receive_handler(const ReceiveHandler& handler)
{
    receive_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取io服务对象
 * 参  数:
 * 返回值: io服务对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
boost::asio::io_service& TcpClientSocketEx::io_service() const
{
    return io_service_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数 
 *         本端数据发送出去后的回调函数
 * 参  数:
 * 返回值: 本端数据发送出去后的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::SentHandler TcpClientSocketEx::sent_handler() const
{
    return sent_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         本端数据发送出去后的回调函数
 * 参  数: [in]handler 本端数据发送出去后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_sent_handler(const SentHandler& handler)
{
    sent_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数 
 *         对端连接上的回调函数
 * 参  数:
 * 返回值: 对端连接上的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::ConnectedHandler TcpClientSocketEx::connected_handler() const
{
	return connected_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         对端连接上的回调函数
 * 参  数: [in]handler 对端连接上的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_connected_handler(const ConnectedHandler& handler)
{
	connected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数 
 *         当对端失去连接后的回调函数
 * 参  数:
 * 返回值: 当对端失去连接后的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::DisconnectedHandler TcpClientSocketEx::disconnected_handler() const
{
	return disconnected_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         当对端失去连接后的回调函数
 * 参  数: [in]handler 当对端失去连接后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_disconnected_handler(const DisconnectedHandler& handler)
{
	disconnected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 *描  述: 连接对端地址
 *参  数:
 *返回值:
 *修  改:
 *  时间 2013.08.23 
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::Connect()
{    
/*
    socket_.async_connect(to_tcp_endpoint(remote_address()), 
        boost::bind(&TcpClientSocketEx::HandleConnect, this,
            boost::asio::placeholders::error));
*/
    boost::system::error_code error; 
    socket_.connect(to_tcp_endpoint(remote_address()), error);
    
    if (!error)
    {
        connected_ = true;

        //从对端接收数据
        socket_.async_read_some(boost::asio::null_buffers(), 
            boost::bind(&TcpClientSocketEx::HandleRead, this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));

       if (connected_handler_)
       {
           connected_handler_();
       }
    }
}

/*------------------------------------------------------------------------------
 *描  述: 当服务器连接上后的回调函数
 *参  数: [in]error 错误代码
 *返回值:
 *修  改:
 *  时间 2013.08.22 
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::HandleConnect(const boost::system::error_code& error)
{
    if (!error) //连接上
    {
        //设置连接上标识
        connected_ = true;

        //从对端接收数据
        socket_.async_read_some(boost::asio::null_buffers(), 
            boost::bind(&TcpClientSocketEx::HandleRead, this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
	else if (disconnected_handler_)
	{
		disconnected_handler_();
	}
}

/*------------------------------------------------------------------------------
 *描  述: 判断是否已经连上对端地址
 *参  数:
 *返回值: 是否已经连上对端地址
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
bool TcpClientSocketEx::IsConnected() const
{
    return connected_;
}

/*------------------------------------------------------------------------------
 *描  述: 当从对端收到数据后的回调函数
 *参  数: [in]error 错误代码
          [in]bytes_transferred 收到的数葑纸谑�
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::HandleRead(const boost::system::error_code& error, 
        uint64 /*bytes_transferred*/)
{
	size_t available = socket_.available(); //可以读取的字节数
	if (error || (available == 0))
	{
		connected_ = false;
        socket_.close();
		if (disconnected_handler_)
		{
			disconnected_handler_();
		}

		return ;
	}

    auto handler = receive_handler(); //接收数据后的回调函数
    if (handler)
    {
        receive_buffer_.resize(available);
        socket_.read_some(boost::asio::buffer(receive_buffer_)); //同步读取数据
        handler(error, (available > 0 ? &receive_buffer_[0] : nullptr), available);
    }
 
    //继续从对端接收数据
    socket_.async_read_some(boost::asio::null_buffers(), 
        boost::bind(&TcpClientSocketEx::HandleRead, this, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * 描  述: 关闭此套接字
 * 参  数:
 * 返回值: 关闭套接字是否成功
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::Close()
{
    socket_.close(); //同步关闭套接字
}

/*------------------------------------------------------------------------------
 *描  述: 向对端发送数据
 *参  数: [in]data 要发送的数据的指针
          [in]length 要发送数据的字节数
 *返回值: 发送是否成功 
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
bool TcpClientSocketEx::Send(const char* data, uint64 length)
{
	if (connected_)
	{
		io_service().post(boost::bind(&TcpClientSocketEx::DoSend, this, data, length));
	}
	
    return connected_;
}

/*------------------------------------------------------------------------------
 *描  述: 向对端发送数据
 *参  数: [in]data 要发送的数据的指针
          [in]length 要发送数据的字节数
 *返回值: 
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::DoSend(const char* data, uint64 length)
{
    boost::asio::async_write(socket_, boost::asio::buffer(data, length), 
        boost::bind(&TcpClientSocketEx::HandleSent, this, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 *描  述: 当数据已经发送成功后的回调函数
 *参  数: [in]error 错误代码
 *返回值: [in]bytes_transferred 数据成功发送的字节数
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::HandleSent(const boost::system::error_code& error, uint64 bytes_transferred)
{
    auto handler = sent_handler();
    if (handler)
    {
        handler(error, bytes_transferred);
    }    
}

/*------------------------------------------------------------------------------
 * 描  述: 获取套接字本端地址
 * 参  数:
 * 返回值: 套接字本端地址
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
EndPoint TcpClientSocketEx::local_address() const
{
    return to_endpoint(socket_.local_endpoint());
}

/*------------------------------------------------------------------------------
 * 描  述: 断线重连的定时器
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::OnTimer()
{
	if (!connected_)
	{
		Connect(); //连接对端
	}
}

}
