/*------------------------------------------------------------------------------
 *文件名   : server_socket.cpp
 *创建人   : rosan
 *创建时间 : 2013.08.22
 *文件描述 : 实现了三个类 ServerSocket TcpServerSocket UdpServerSocket
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#include "server_socket.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: ServerSocket的构造函数
 *参  数: [in]local_port 本端套接字绑定的端口号
 *        [in]ios IO服务对象
 *        [in]handler 当从对端收到数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
ServerSocket::ServerSocket(unsigned short local_port, boost::asio::io_service& ios,
    const ReceiveHandler& handler) :
    local_address_(local_port, boost::asio::ip::address_v4::any().to_ulong()), 
    io_service_(ios),
    receive_handler_(handler)
{ 
}

/*------------------------------------------------------------------------------
 *描  述: ServerSocket的构造函数
 *参  数: [in]local_address 套接字绑定本地地址
 *        [in]ios IO服务对象
 *        [in]handler 当从对端收到数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
ServerSocket::ServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    local_address_(local_address.port, boost::asio::ip::address_v4::any().to_ulong()), 
    io_service_(ios),
    receive_handler_(handler)
{
}

/*------------------------------------------------------------------------------
 * 描  述: ServerSocket类的析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
ServerSocket::~ServerSocket()
{
}

/*------------------------------------------------------------------------------
 * 描  述: 获取套接字本地地址
 * 参  数:
 * 返回值: 套接字本地地址
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
EndPoint ServerSocket::local_address() const
{
    return local_address_;
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
boost::asio::io_service& ServerSocket::io_service() const
{
    return io_service_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数
 *         从对端收到数据后的回调函数
 * 参  数:
 * 返回值: 从对端收到数据后的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
ServerSocket::ReceiveHandler ServerSocket::receive_handler() const
{
    return receive_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         从对端收到数据后的回调函数
 * 参  数:
 * 返回值: 从对端收到数据后的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void ServerSocket::set_receive_handler(const ReceiveHandler& handler)
{
    receive_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数
 *         向对端发送数据后的回调函数
 * 参  数:
 * 返回值: 向对端发送数据后的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
ServerSocket::SentHandler ServerSocket::sent_handler() const
{
    return sent_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         向对端发送数据后的回调函数 
 * 参  数: [in]handler 向对端发送数据后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void ServerSocket::set_sent_handler(const SentHandler& handler)
{
    sent_handler_ = handler;
}

/*------------------------------------------------------------------------------
 *描  述: TcpServerSocket的构造函数
 *参  数: [in]local_port 套接字绑定的本地端口号
 *        [in]ios IO服务对象
 *        [in]handler 当从对端接收到数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
TcpServerSocket::TcpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_port, ios, handler),
    acceptor_(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), local_port), true)
{
    //acceptor_.set_option(boost::asio::socket_base::reuse_address(true));  // 设置地址重用

    StartAccept(); //接收来自对端的连接
}

/*------------------------------------------------------------------------------
 *描  述: TcpServerSocket的构造函数
 *参  数: [in]local_address 服务器绑定的本地地址
 *        [in]ios IO服务对象
 *        [in]handler 当从对端收到数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
TcpServerSocket::TcpServerSocket(const EndPoint& local_addr, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_addr, ios, handler),
    acceptor_(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), local_addr.port))
{
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));  // 设置地址重用

    StartAccept(); //接收来自对端的连接
}

/*------------------------------------------------------------------------------
 *描  述: TcpServerSoket的析构函数
 *参  数:
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
TcpServerSocket::~TcpServerSocket()
{
    acceptor_.close(); //同步关闭接收器
    for (decltype(*sessions_.begin()) item : sessions_)
    {
        RemoveSession(item.first); //关闭每一个会话(socket), 删除创建的会话
    } 
}

/*------------------------------------------------------------------------------
 *描  述: 类TcpServerSocket::Session的构造函数
 *参  数: [in]ios IO服务对象
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
TcpServerSocket::Session::Session(boost::asio::io_service& ios) : socket(ios)
{
}

/*------------------------------------------------------------------------------
 *描  述: 等待来自客户端的连接
 *参  数:
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
void TcpServerSocket::StartAccept()
{
    //创建一个会话(socket)用于和客户端进行连接，通信
    Session* new_session = new Session(io_service());

    //等待来自客户端的连接
    acceptor_.async_accept(new_session->socket,
        boost::bind(&TcpServerSocket::HandleAccept, this, new_session,
            boost::asio::placeholders::error));
}

/*------------------------------------------------------------------------------
 *描  述: 接收来自对端的连接
 *参  数: [in]new_session 这个连接对应的会话(socket)
          [in]error 错误代码
 *返回值: 
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
void TcpServerSocket::HandleAccept(Session* new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        auto socket = &new_session->socket; //会话对应的socket
        EndPoint endpoint = to_endpoint(socket->remote_endpoint()); //客户端地址

        if (connected_handler_)
        {
            connected_handler_(endpoint);
        }

        if (sessions_.find(endpoint) == sessions_.end()) //对端之前没有连接上服务器
        {
            sessions_.insert(std::make_pair(endpoint, new_session)); //保存会话

            //接收来自客户端的数据
            socket->async_read_some(boost::asio::null_buffers(),
                boost::bind(&TcpServerSocket::HandleRead, this, endpoint,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)); 
        } 
    }
    else //连接出错
    {
        delete new_session; //删除会话
    }

    StartAccept(); //继续等待来自客户端的连接
}

/*------------------------------------------------------------------------------
 *描  述: 删除一个对端会话(socket连接)
 *参  数: [in]endpoint 对端地址
 *返回值: 删除是否成功
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
bool TcpServerSocket::RemoveSession(const EndPoint& endpoint)
{
    if (disconnected_handler_)
    {
        disconnected_handler_(endpoint);
    }

    auto i = sessions_.find(endpoint);
    if (i != sessions_.end()) //判断会话是否存在
    {
        i->second->socket.close(); //关闭会话对应的socket
        delete i->second; //删除会话对象
        sessions_.erase(i); //将会话从列表中删除

        return true;
    }

    return false;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数
 *         对端连接上后的回调函数
 * 参  数:
 * 返回值: 获取回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TcpServerSocket::ConnectedHandler TcpServerSocket::connected_handler() const
{
    return connected_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         对端连接上后的回调函数
 * 参  数: [in]handler 对端连接上后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpServerSocket::set_connected_handler(const ConnectedHandler& handler)
{
    connected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取回调函数
 *         对端失去连接后的回调函数
 * 参  数:
 * 返回值: 对端失去连接后的回调函数
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TcpServerSocket::DisconnectedHandler TcpServerSocket::disconnected_handler() const
{
    return disconnected_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置回调函数
 *         对端失去连接后的回调函数
 * 参  数: [in]handler 对端失去连接后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpServerSocket::set_disconnected_handler(const DisconnectedHandler& handler)
{
    disconnected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取连接上此套接字的对端地址列表
 * 参  数:
 * 返回值: 连接上此套接字的对端地址列表
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
boost::unordered_set<EndPoint> TcpServerSocket::GetClients() const
{
    boost::unordered_set<EndPoint> clients;
    for (decltype(*sessions_.begin())& i : sessions_)
    {
        clients.insert(i.first);
    }

    return clients;
}

/*------------------------------------------------------------------------------
 *描  述: 当从对端接收到数据后的回调函数
 *参  数: [in]endpoint 对端地址
          [in]error 错误代码
          [in]bytes_transferred 收到数据的字节数
 *返回值: 
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
void TcpServerSocket::HandleRead(const EndPoint& endpoint, const boost::system::error_code& error, 
    uint64 bytes_transferred)
{
    Session* session = sessions_[endpoint]; //客户端地址对应的会话
    auto& sock = session->socket; //会话的套接字
    auto& buf = session->receive_buffer; //会话接收数据缓冲区
    size_t available = sock.available();

    if (error || (available == 0))
    {
        RemoveSession(endpoint);
        return ;
    }

    auto handler = receive_handler();
    if (handler)
    {
        size_t available = sock.available(); //可以读取的字节数
        buf.resize(available);
        sock.read_some(boost::asio::buffer(buf)); //读取数据
        handler(endpoint, error, (available > 0 ? &buf[0] : nullptr), available);
    }

    //继续接收来自客户端的数据
    sock.async_read_some(boost::asio::null_buffers(),
        boost::bind(&TcpServerSocket::HandleRead, this, endpoint,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *描  述: 向对端发送数据
 *参  数: [in]remote_address 对端地址
          [in]data 发送的数据指针
          [in]length 发送的字节数
 *返回值: 发送数据是否成功
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
bool TcpServerSocket::Send(const EndPoint& remote_address, const char* data, uint64 length)
{
    auto i = sessions_.find(remote_address);
    if (i != sessions_.end()) //判断连接是否存在
    {
        //发送数据给对端
        io_service().post(boost::bind(&TcpServerSocket::DoSend, this, &i->second->socket, remote_address, data, length));
        return true;
    }
    
    return false;
}

/*------------------------------------------------------------------------------
 * 描  述: 向对端发送数据
 * 参  数: [in]socket 与对端通信的套接字
 *         [in]remote_address 对端地址
 *         [in]data 发送给对端的数据
 *         [in]length 发送给对端数据的长度
 * 返回值:
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TcpServerSocket::DoSend(boost::asio::ip::tcp::socket* socket, 
        const EndPoint& remote_address, const char* data, uint64 length)
{
    //发送数据给客户端
     boost::asio::async_write(*socket, boost::asio::buffer(data, length),
        boost::bind(&TcpServerSocket::HandleWrite, this, remote_address,
             boost::asio::placeholders::error,
             boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 *描  述: 当数据发送出去后的回调函数
 *参  数: [in]remote_address 对端地址
          [in]error 错误代码
          [in]bytes_transferred 成功发送的字节数
 *返回值: 
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
void TcpServerSocket::HandleWrite(const EndPoint& remote_address, 
    const boost::system::error_code& error, uint64 bytes_transferred)
{
    auto handler = sent_handler();
    if (handler)
    {
        handler(remote_address, error, bytes_transferred);
    }

    if (error) //出错
    {
        RemoveSession(remote_address); //删除此会话
    }
}

/*------------------------------------------------------------------------------
 *描  述: UdpServerSocket的构造函数
 *参  数: [in]local_port 套接字绑定的本地端口号
 *        [in]ios IO服务对象
 *        [in]handler 当从对端收到数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
UdpServerSocket::UdpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_port, ios, handler),
    socket_(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), local_port)) 
{
    socket_.set_option(boost::asio::socket_base::reuse_address(true));  // 设置地址重用

    socket_.async_receive_from(boost::asio::null_buffers(), sender_endpoint_,
        boost::bind(&UdpServerSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *描  述: UdpServerSocket的构造函数
 *参  数: [in]local_address 套接字绑定的本地地址
          [in]ios IO服务对象
          [in]handler 当从对端收到数据后的回调函数
 *返回值:
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
UdpServerSocket::UdpServerSocket(const EndPoint& local_addr, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_addr, ios, handler),
    socket_(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), local_addr.port)) 
{
    socket_.set_option(boost::asio::socket_base::reuse_address(true));  // 设置地址重用

    socket_.async_receive_from(boost::asio::null_buffers(), sender_endpoint_,
        boost::bind(&UdpServerSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *描  述: UdpServerSocket的析构函数
 *参  数:
 *返回值: 
 *修  改:
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
UdpServerSocket::~UdpServerSocket()
{
    socket_.close(); //同步关闭套接字
}

/*------------------------------------------------------------------------------
 *描  述: 当接收来自客户端数据后的回调函数
 *参  数: [in]error 错误代码
 *        [in]bytes_transffered 收到数据的字节数
 *返回值: 
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
void UdpServerSocket::HandleReceiveFrom(const boost::system::error_code& error, 
        uint64 /*bytes_transferred*/)
{
    auto handler = receive_handler();
    if (handler)
    {
        size_t available = socket_.available(); //可以读取的字节数
        receive_buffer_.resize(available);
        socket_.receive(boost::asio::buffer(receive_buffer_)); //读取数据
        handler(to_endpoint(sender_endpoint_), error, 
            (available > 0 ? &receive_buffer_[0] : nullptr), available);
    } 
   
    //继续接收来自客户端的数据
    socket_.async_receive_from(boost::asio::null_buffers(), sender_endpoint_,
        boost::bind(&UdpServerSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *描  述: 向对端发送数据
 *参  数: [in]remote_addres 对端地址
 *        [in]data 发送的数据
 *        [in]length 发送数据的字节数
 *返回值: 发送数据是否成功 
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
bool UdpServerSocket::Send(const EndPoint& remote_address, const char* data, uint64 length)
{
    io_service().post(boost::bind(&UdpServerSocket::DoSend, this, remote_address, data, length));
    return true;
}

/*------------------------------------------------------------------------------
 *描  述: 向对端发送数据
 *参  数: [in]remote_addres 对端地址
 *        [in]data 发送的数据
 *        [in]length 发送数据的字节数
 *返回值:
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
void UdpServerSocket::DoSend(const EndPoint& remote_address, const char* data, uint64 length)
{
    //发送数据到客户端
    socket_.async_send_to(boost::asio::buffer(data, length), 
        to_udp_endpoint(remote_address),
        boost::bind(&UdpServerSocket::HandleSendTo, this, remote_address,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 *描  述: 当数据发送到客户端后的回调函数
 *参  数: [in]remote_address 对端地址
          [in]error 错误代码
          [in]bytes_transferred 成功发送的字节数
 *返回值: 
 *修  改:          
 *  时间 2013.08.22
 *  作者 rosan
 *  描述 创建
 -----------------------------------------------------------------------------*/
void UdpServerSocket::HandleSendTo(const EndPoint& remote_address, const boost::system::error_code& error,
        uint64 bytes_transferred)
{
    auto handler = sent_handler();
    if (handler)
    {
        handler(remote_address, error, bytes_transferred);
    }
}

}
