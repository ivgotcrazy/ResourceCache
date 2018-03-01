/*------------------------------------------------------------------------------
 *�ļ���   : server_socket.cpp
 *������   : rosan
 *����ʱ�� : 2013.08.22
 *�ļ����� : ʵ���������� ServerSocket TcpServerSocket UdpServerSocket
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#include "server_socket.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *��  ��: ServerSocket�Ĺ��캯��
 *��  ��: [in]local_port �����׽��ְ󶨵Ķ˿ں�
 *        [in]ios IO�������
 *        [in]handler ���ӶԶ��յ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
ServerSocket::ServerSocket(unsigned short local_port, boost::asio::io_service& ios,
    const ReceiveHandler& handler) :
    local_address_(local_port, boost::asio::ip::address_v4::any().to_ulong()), 
    io_service_(ios),
    receive_handler_(handler)
{ 
}

/*------------------------------------------------------------------------------
 *��  ��: ServerSocket�Ĺ��캯��
 *��  ��: [in]local_address �׽��ְ󶨱��ص�ַ
 *        [in]ios IO�������
 *        [in]handler ���ӶԶ��յ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
ServerSocket::ServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    local_address_(local_address.port, boost::asio::ip::address_v4::any().to_ulong()), 
    io_service_(ios),
    receive_handler_(handler)
{
}

/*------------------------------------------------------------------------------
 * ��  ��: ServerSocket�����������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
ServerSocket::~ServerSocket()
{
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�׽��ֱ��ص�ַ
 * ��  ��:
 * ����ֵ: �׽��ֱ��ص�ַ
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
EndPoint ServerSocket::local_address() const
{
    return local_address_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡio�������
 * ��  ��:
 * ����ֵ: io�������
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
boost::asio::io_service& ServerSocket::io_service() const
{
    return io_service_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص�����
 *         �ӶԶ��յ����ݺ�Ļص�����
 * ��  ��:
 * ����ֵ: �ӶԶ��յ����ݺ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
ServerSocket::ReceiveHandler ServerSocket::receive_handler() const
{
    return receive_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         �ӶԶ��յ����ݺ�Ļص�����
 * ��  ��:
 * ����ֵ: �ӶԶ��յ����ݺ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void ServerSocket::set_receive_handler(const ReceiveHandler& handler)
{
    receive_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص�����
 *         ��Զ˷������ݺ�Ļص�����
 * ��  ��:
 * ����ֵ: ��Զ˷������ݺ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
ServerSocket::SentHandler ServerSocket::sent_handler() const
{
    return sent_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         ��Զ˷������ݺ�Ļص����� 
 * ��  ��: [in]handler ��Զ˷������ݺ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void ServerSocket::set_sent_handler(const SentHandler& handler)
{
    sent_handler_ = handler;
}

/*------------------------------------------------------------------------------
 *��  ��: TcpServerSocket�Ĺ��캯��
 *��  ��: [in]local_port �׽��ְ󶨵ı��ض˿ں�
 *        [in]ios IO�������
 *        [in]handler ���ӶԶ˽��յ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
TcpServerSocket::TcpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_port, ios, handler),
    acceptor_(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), local_port), true)
{
    //acceptor_.set_option(boost::asio::socket_base::reuse_address(true));  // ���õ�ַ����

    StartAccept(); //�������ԶԶ˵�����
}

/*------------------------------------------------------------------------------
 *��  ��: TcpServerSocket�Ĺ��캯��
 *��  ��: [in]local_address �������󶨵ı��ص�ַ
 *        [in]ios IO�������
 *        [in]handler ���ӶԶ��յ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
TcpServerSocket::TcpServerSocket(const EndPoint& local_addr, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_addr, ios, handler),
    acceptor_(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), local_addr.port))
{
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));  // ���õ�ַ����

    StartAccept(); //�������ԶԶ˵�����
}

/*------------------------------------------------------------------------------
 *��  ��: TcpServerSoket����������
 *��  ��:
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
TcpServerSocket::~TcpServerSocket()
{
    acceptor_.close(); //ͬ���رս�����
    for (decltype(*sessions_.begin()) item : sessions_)
    {
        RemoveSession(item.first); //�ر�ÿһ���Ự(socket), ɾ�������ĻỰ
    } 
}

/*------------------------------------------------------------------------------
 *��  ��: ��TcpServerSocket::Session�Ĺ��캯��
 *��  ��: [in]ios IO�������
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
TcpServerSocket::Session::Session(boost::asio::io_service& ios) : socket(ios)
{
}

/*------------------------------------------------------------------------------
 *��  ��: �ȴ����Կͻ��˵�����
 *��  ��:
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
void TcpServerSocket::StartAccept()
{
    //����һ���Ự(socket)���ںͿͻ��˽������ӣ�ͨ��
    Session* new_session = new Session(io_service());

    //�ȴ����Կͻ��˵�����
    acceptor_.async_accept(new_session->socket,
        boost::bind(&TcpServerSocket::HandleAccept, this, new_session,
            boost::asio::placeholders::error));
}

/*------------------------------------------------------------------------------
 *��  ��: �������ԶԶ˵�����
 *��  ��: [in]new_session ������Ӷ�Ӧ�ĻỰ(socket)
          [in]error �������
 *����ֵ: 
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
void TcpServerSocket::HandleAccept(Session* new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        auto socket = &new_session->socket; //�Ự��Ӧ��socket
        EndPoint endpoint = to_endpoint(socket->remote_endpoint()); //�ͻ��˵�ַ

        if (connected_handler_)
        {
            connected_handler_(endpoint);
        }

        if (sessions_.find(endpoint) == sessions_.end()) //�Զ�֮ǰû�������Ϸ�����
        {
            sessions_.insert(std::make_pair(endpoint, new_session)); //����Ự

            //�������Կͻ��˵�����
            socket->async_read_some(boost::asio::null_buffers(),
                boost::bind(&TcpServerSocket::HandleRead, this, endpoint,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)); 
        } 
    }
    else //���ӳ���
    {
        delete new_session; //ɾ���Ự
    }

    StartAccept(); //�����ȴ����Կͻ��˵�����
}

/*------------------------------------------------------------------------------
 *��  ��: ɾ��һ���Զ˻Ự(socket����)
 *��  ��: [in]endpoint �Զ˵�ַ
 *����ֵ: ɾ���Ƿ�ɹ�
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
bool TcpServerSocket::RemoveSession(const EndPoint& endpoint)
{
    if (disconnected_handler_)
    {
        disconnected_handler_(endpoint);
    }

    auto i = sessions_.find(endpoint);
    if (i != sessions_.end()) //�жϻỰ�Ƿ����
    {
        i->second->socket.close(); //�رջỰ��Ӧ��socket
        delete i->second; //ɾ���Ự����
        sessions_.erase(i); //���Ự���б���ɾ��

        return true;
    }

    return false;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص�����
 *         �Զ������Ϻ�Ļص�����
 * ��  ��:
 * ����ֵ: ��ȡ�ص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
TcpServerSocket::ConnectedHandler TcpServerSocket::connected_handler() const
{
    return connected_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         �Զ������Ϻ�Ļص�����
 * ��  ��: [in]handler �Զ������Ϻ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpServerSocket::set_connected_handler(const ConnectedHandler& handler)
{
    connected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص�����
 *         �Զ�ʧȥ���Ӻ�Ļص�����
 * ��  ��:
 * ����ֵ: �Զ�ʧȥ���Ӻ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
TcpServerSocket::DisconnectedHandler TcpServerSocket::disconnected_handler() const
{
    return disconnected_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         �Զ�ʧȥ���Ӻ�Ļص�����
 * ��  ��: [in]handler �Զ�ʧȥ���Ӻ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpServerSocket::set_disconnected_handler(const DisconnectedHandler& handler)
{
    disconnected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�����ϴ��׽��ֵĶԶ˵�ַ�б�
 * ��  ��:
 * ����ֵ: �����ϴ��׽��ֵĶԶ˵�ַ�б�
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
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
 *��  ��: ���ӶԶ˽��յ����ݺ�Ļص�����
 *��  ��: [in]endpoint �Զ˵�ַ
          [in]error �������
          [in]bytes_transferred �յ����ݵ��ֽ���
 *����ֵ: 
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
void TcpServerSocket::HandleRead(const EndPoint& endpoint, const boost::system::error_code& error, 
    uint64 bytes_transferred)
{
    Session* session = sessions_[endpoint]; //�ͻ��˵�ַ��Ӧ�ĻỰ
    auto& sock = session->socket; //�Ự���׽���
    auto& buf = session->receive_buffer; //�Ự�������ݻ�����
    size_t available = sock.available();

    if (error || (available == 0))
    {
        RemoveSession(endpoint);
        return ;
    }

    auto handler = receive_handler();
    if (handler)
    {
        size_t available = sock.available(); //���Զ�ȡ���ֽ���
        buf.resize(available);
        sock.read_some(boost::asio::buffer(buf)); //��ȡ����
        handler(endpoint, error, (available > 0 ? &buf[0] : nullptr), available);
    }

    //�����������Կͻ��˵�����
    sock.async_read_some(boost::asio::null_buffers(),
        boost::bind(&TcpServerSocket::HandleRead, this, endpoint,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *��  ��: ��Զ˷�������
 *��  ��: [in]remote_address �Զ˵�ַ
          [in]data ���͵�����ָ��
          [in]length ���͵��ֽ���
 *����ֵ: ���������Ƿ�ɹ�
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
bool TcpServerSocket::Send(const EndPoint& remote_address, const char* data, uint64 length)
{
    auto i = sessions_.find(remote_address);
    if (i != sessions_.end()) //�ж������Ƿ����
    {
        //�������ݸ��Զ�
        io_service().post(boost::bind(&TcpServerSocket::DoSend, this, &i->second->socket, remote_address, data, length));
        return true;
    }
    
    return false;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��Զ˷�������
 * ��  ��: [in]socket ��Զ�ͨ�ŵ��׽���
 *         [in]remote_address �Զ˵�ַ
 *         [in]data ���͸��Զ˵�����
 *         [in]length ���͸��Զ����ݵĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpServerSocket::DoSend(boost::asio::ip::tcp::socket* socket, 
        const EndPoint& remote_address, const char* data, uint64 length)
{
    //�������ݸ��ͻ���
     boost::asio::async_write(*socket, boost::asio::buffer(data, length),
        boost::bind(&TcpServerSocket::HandleWrite, this, remote_address,
             boost::asio::placeholders::error,
             boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 *��  ��: �����ݷ��ͳ�ȥ��Ļص�����
 *��  ��: [in]remote_address �Զ˵�ַ
          [in]error �������
          [in]bytes_transferred �ɹ����͵��ֽ���
 *����ֵ: 
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
void TcpServerSocket::HandleWrite(const EndPoint& remote_address, 
    const boost::system::error_code& error, uint64 bytes_transferred)
{
    auto handler = sent_handler();
    if (handler)
    {
        handler(remote_address, error, bytes_transferred);
    }

    if (error) //����
    {
        RemoveSession(remote_address); //ɾ���˻Ự
    }
}

/*------------------------------------------------------------------------------
 *��  ��: UdpServerSocket�Ĺ��캯��
 *��  ��: [in]local_port �׽��ְ󶨵ı��ض˿ں�
 *        [in]ios IO�������
 *        [in]handler ���ӶԶ��յ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
UdpServerSocket::UdpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_port, ios, handler),
    socket_(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), local_port)) 
{
    socket_.set_option(boost::asio::socket_base::reuse_address(true));  // ���õ�ַ����

    socket_.async_receive_from(boost::asio::null_buffers(), sender_endpoint_,
        boost::bind(&UdpServerSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *��  ��: UdpServerSocket�Ĺ��캯��
 *��  ��: [in]local_address �׽��ְ󶨵ı��ص�ַ
          [in]ios IO�������
          [in]handler ���ӶԶ��յ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
UdpServerSocket::UdpServerSocket(const EndPoint& local_addr, boost::asio::io_service& ios,
    const ReceiveHandler& handler) : 
    ServerSocket(local_addr, ios, handler),
    socket_(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), local_addr.port)) 
{
    socket_.set_option(boost::asio::socket_base::reuse_address(true));  // ���õ�ַ����

    socket_.async_receive_from(boost::asio::null_buffers(), sender_endpoint_,
        boost::bind(&UdpServerSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *��  ��: UdpServerSocket����������
 *��  ��:
 *����ֵ: 
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
UdpServerSocket::~UdpServerSocket()
{
    socket_.close(); //ͬ���ر��׽���
}

/*------------------------------------------------------------------------------
 *��  ��: ���������Կͻ������ݺ�Ļص�����
 *��  ��: [in]error �������
 *        [in]bytes_transffered �յ����ݵ��ֽ���
 *����ֵ: 
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
void UdpServerSocket::HandleReceiveFrom(const boost::system::error_code& error, 
        uint64 /*bytes_transferred*/)
{
    auto handler = receive_handler();
    if (handler)
    {
        size_t available = socket_.available(); //���Զ�ȡ���ֽ���
        receive_buffer_.resize(available);
        socket_.receive(boost::asio::buffer(receive_buffer_)); //��ȡ����
        handler(to_endpoint(sender_endpoint_), error, 
            (available > 0 ? &receive_buffer_[0] : nullptr), available);
    } 
   
    //�����������Կͻ��˵�����
    socket_.async_receive_from(boost::asio::null_buffers(), sender_endpoint_,
        boost::bind(&UdpServerSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred)); 
}

/*------------------------------------------------------------------------------
 *��  ��: ��Զ˷�������
 *��  ��: [in]remote_addres �Զ˵�ַ
 *        [in]data ���͵�����
 *        [in]length �������ݵ��ֽ���
 *����ֵ: ���������Ƿ�ɹ� 
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
bool UdpServerSocket::Send(const EndPoint& remote_address, const char* data, uint64 length)
{
    io_service().post(boost::bind(&UdpServerSocket::DoSend, this, remote_address, data, length));
    return true;
}

/*------------------------------------------------------------------------------
 *��  ��: ��Զ˷�������
 *��  ��: [in]remote_addres �Զ˵�ַ
 *        [in]data ���͵�����
 *        [in]length �������ݵ��ֽ���
 *����ֵ:
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/
void UdpServerSocket::DoSend(const EndPoint& remote_address, const char* data, uint64 length)
{
    //�������ݵ��ͻ���
    socket_.async_send_to(boost::asio::buffer(data, length), 
        to_udp_endpoint(remote_address),
        boost::bind(&UdpServerSocket::HandleSendTo, this, remote_address,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 *��  ��: �����ݷ��͵��ͻ��˺�Ļص�����
 *��  ��: [in]remote_address �Զ˵�ַ
          [in]error �������
          [in]bytes_transferred �ɹ����͵��ֽ���
 *����ֵ: 
 *��  ��:          
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
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
