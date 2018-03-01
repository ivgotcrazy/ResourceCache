/*------------------------------------------------------------------------------
 * 文件名   : client_socket.cpp 
 * 创建人   : rosan 
 * 创建时间 : 2013.08.22
 * 文件描述 : 实现了三个类ClientSocket TcpClientSocket UdpClientSocket 
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#include "client_socket.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: ClientSocket构造函数
 *参  数: [in] ios IO服务
        : [in] remote_address 服务器地址
        : [in] handler 接收到服务器数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
ClientSocket::ClientSocket(boost::asio::io_service& ios,
    const EndPoint& remote_address,
    const ReceiveHandler& handler) : 
    remote_address_(remote_address), 
    receive_handler_(handler),
    io_service_(ios) 
{ 
}

/*------------------------------------------------------------------------------
 *描  述: ClientSocket的析构函数
 *参  数:
 *返回值:
 *修  改:
 *  时间 2013.08.20 
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
ClientSocket::~ClientSocket()
{
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
EndPoint ClientSocket::remote_address() const
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
ClientSocket::ReceiveHandler ClientSocket::receive_handler() const
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
void ClientSocket::set_receive_handler(const ReceiveHandler& handler)
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
boost::asio::io_service& ClientSocket::io_service() const
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
ClientSocket::SentHandler ClientSocket::sent_handler() const
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
void ClientSocket::set_sent_handler(const SentHandler& handler)
{
    sent_handler_ = handler;
}

/*------------------------------------------------------------------------------
 *描  述: TcpClientSocket构造函数
 *参  数: [in]ios IO服务
        : [in]remote_address 对端地址
        : [in]handler 接收到对端数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocket::TcpClientSocket(boost::asio::io_service& ios,
    const EndPoint& remote_address,
    const ReceiveHandler& handler) : 
    ClientSocket(ios, remote_address, handler),
    connected_(false),
    socket_(ios, boost::asio::ip::tcp::endpoint(
        boost::asio::ip::tcp::v4(), 0))
{
    Connect(); //连接对端
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
void TcpClientSocket::Connect()
{    
    socket_.async_connect(to_tcp_endpoint(remote_address()), 
        boost::bind(&TcpClientSocket::HandleConnect, this,
            boost::asio::placeholders::error));
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
void TcpClientSocket::HandleConnect(const boost::system::error_code& error)
{
    if (!error) //连接上
    {
        //设置连接上标识
        connected_ = true;

        //从对端接收数据
        socket_.async_read_some(boost::asio::null_buffers(), 
            boost::bind(&TcpClientSocket::HandleRead, this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else //没有连接上
    {
        Connect(); //继续连接
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
bool TcpClientSocket::IsConnected() const
{
    return connected_;
}

/*------------------------------------------------------------------------------
 *描  述: 当从对端收到数据后的回调函数
 *参  数: [in]error 错误代码
          [in]bytes_transferred 收到的数据字节数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpClientSocket::HandleRead(const boost::system::error_code& error, 
        uint64 /*bytes_transferred*/)
{
    auto handler = receive_handler(); //接收数据后的回调函数
    if (handler)
    {
        size_t available = socket_.available(); //可以读取的字节数
        receive_buffer_.resize(available);
        socket_.read_some(boost::asio::buffer(receive_buffer_)); //同步读取数据
        handler(error, (available > 0 ? &receive_buffer_[0] : nullptr), available);
    }
 
    //继续从对端接收数据
    socket_.async_read_some(boost::asio::null_buffers(), 
        boost::bind(&TcpClientSocket::HandleRead, this, 
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
bool TcpClientSocket::Close()
{
    socket_.close(); //同步关闭套接字

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: TcpClientSocket析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TcpClientSocket::~TcpClientSocket()
{
    Close(); //关闭套接字
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
bool TcpClientSocket::Send(const char* data, uint64 length)
{
    io_service().post(boost::bind(&TcpClientSocket::DoSend, this, data, length));    

    return true;
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
void TcpClientSocket::DoSend(const char* data, uint64 length)
{
    boost::asio::async_write(socket_, boost::asio::buffer(data, length), 
        boost::bind(&TcpClientSocket::HandleSent, this, 
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
void TcpClientSocket::HandleSent(const boost::system::error_code& error, uint64 bytes_transferred)
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
EndPoint TcpClientSocket::local_address() const
{
    return to_endpoint(socket_.local_endpoint());
}

/*------------------------------------------------------------------------------
 *描  述: UdpClientSocket构造函数
 *参  数: [in]ios IO服务
          [in]remote_address 服务器地址
          [in]handler 接收到对端数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
UdpClientSocket::UdpClientSocket(boost::asio::io_service& ios,
    const EndPoint& remote_address,
    const ReceiveHandler& handler) : 
    ClientSocket(ios, remote_address, handler),
    socket_(ios, boost::asio::ip::udp::endpoint(
        boost::asio::ip::udp::v4(), 0))
{
    endpoint_ = to_udp_endpoint(remote_address);

    //从服务器接收数据
    socket_.async_receive_from(boost::asio::null_buffers(), endpoint_, 
        boost::bind(&UdpClientSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * 描  述: UdpClientSocket类的析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
UdpClientSocket::~UdpClientSocket()
{
    Close(); //关闭此套接字
}

/*------------------------------------------------------------------------------
 *描  述: 向对端发送数据
 *参  数: [in]data 要发送数据的指针
          [in]length 要发送数据的字节数
 *返回值: 发送数据是否成功
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
bool UdpClientSocket::Send(const char* data, uint64 length)
{
    io_service().post(boost::bind(&UdpClientSocket::DoSend, this, data, length));

    return true;
}

/*------------------------------------------------------------------------------
 *描  述: 向对端发送数据
 *参  数: [in]data 要发送数据的指针
          [in]length 要发送数据的字节数
 *返回值: 
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void UdpClientSocket::DoSend(const char* data, uint64 length)
{    
    socket_.async_send_to(boost::asio::buffer(data, length), endpoint_,
        boost::bind(&UdpClientSocket::HandleSendTo, this, 
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
bool UdpClientSocket::Close()
{
    socket_.close(); //关闭套接字

    return true;
}

/*------------------------------------------------------------------------------
 *描  述: 当数据发送完成后的回调函数
 *参  数: [in]error 错误代码
          [in]bytes_transferred 实际发送的字节数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void UdpClientSocket::HandleSendTo(const boost::system::error_code& error, 
        uint64 bytes_transferred)
{
    auto handler = sent_handler();
    if (handler)
    {
        handler(error, bytes_transferred);
    }
}

/*------------------------------------------------------------------------------
 *描  述: 当从对端接收到数据后的回调函数
 *参  数: [in]error 错误代码
          [in]bytes_transferred 接收到的数据字节数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/ 
void UdpClientSocket::HandleReceiveFrom(const boost::system::error_code& error, uint64 /*bytes_transferred*/)
{
    auto handler = receive_handler(); //接收到数据后的回调函数
    if (handler)
    {
        size_t available = socket_.available(); //可以读取的字节数
        receive_buffer_.resize(available); 
        socket_.receive(boost::asio::buffer(receive_buffer_)); //读取数据
        handler(error, (available > 0 ? &receive_buffer_[0] : nullptr), available);
    }
    
    //继续接收数据
    socket_.async_receive_from(boost::asio::null_buffers(), endpoint_, 
        boost::bind(&UdpClientSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * 描  述: 获取本端套接字地址
 * 参  数:
 * 返回值: 本端套接字地址
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
EndPoint UdpClientSocket::local_address() const
{
    return to_endpoint(socket_.local_endpoint());
}

}
