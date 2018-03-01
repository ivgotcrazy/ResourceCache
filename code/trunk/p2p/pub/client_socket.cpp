/*------------------------------------------------------------------------------
 * �ļ���   : client_socket.cpp 
 * ������   : rosan 
 * ����ʱ�� : 2013.08.22
 * �ļ����� : ʵ����������ClientSocket TcpClientSocket UdpClientSocket 
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#include "client_socket.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *��  ��: ClientSocket���캯��
 *��  ��: [in] ios IO����
        : [in] remote_address ��������ַ
        : [in] handler ���յ����������ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
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
 *��  ��: ClientSocket����������
 *��  ��:
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.20 
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
ClientSocket::~ClientSocket()
{
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡsocket�Զ˵ĵ�ַ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
EndPoint ClientSocket::remote_address() const
{
    return remote_address_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص�����
 *         �˻ص������ǽ��նԶ����ݺ�Ĵ�����
 * ��  ��:
 * ����ֵ: ���նԶ����ݺ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
ClientSocket::ReceiveHandler ClientSocket::receive_handler() const
{
    return receive_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         �˻ص������ǽ��նԶ����ݺ�Ĵ�����
 * ��  ��: [in]handler ���նԶ����ݺ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void ClientSocket::set_receive_handler(const ReceiveHandler& handler)
{
    receive_handler_ = handler;
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
boost::asio::io_service& ClientSocket::io_service() const
{
    return io_service_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص����� 
 *         �������ݷ��ͳ�ȥ��Ļص�����
 * ��  ��:
 * ����ֵ: �������ݷ��ͳ�ȥ��Ļص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
ClientSocket::SentHandler ClientSocket::sent_handler() const
{
    return sent_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         �������ݷ��ͳ�ȥ��Ļص�����
 * ��  ��: [in]handler �������ݷ��ͳ�ȥ��Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void ClientSocket::set_sent_handler(const SentHandler& handler)
{
    sent_handler_ = handler;
}

/*------------------------------------------------------------------------------
 *��  ��: TcpClientSocket���캯��
 *��  ��: [in]ios IO����
        : [in]remote_address �Զ˵�ַ
        : [in]handler ���յ��Զ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
TcpClientSocket::TcpClientSocket(boost::asio::io_service& ios,
    const EndPoint& remote_address,
    const ReceiveHandler& handler) : 
    ClientSocket(ios, remote_address, handler),
    connected_(false),
    socket_(ios, boost::asio::ip::tcp::endpoint(
        boost::asio::ip::tcp::v4(), 0))
{
    Connect(); //���ӶԶ�
}

/*------------------------------------------------------------------------------
 *��  ��: ���ӶԶ˵�ַ
 *��  ��:
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.23 
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocket::Connect()
{    
    socket_.async_connect(to_tcp_endpoint(remote_address()), 
        boost::bind(&TcpClientSocket::HandleConnect, this,
            boost::asio::placeholders::error));
}

/*------------------------------------------------------------------------------
 *��  ��: �������������Ϻ�Ļص�����
 *��  ��: [in]error �������
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22 
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocket::HandleConnect(const boost::system::error_code& error)
{
    if (!error) //������
    {
        //���������ϱ�ʶ
        connected_ = true;

        //�ӶԶ˽�������
        socket_.async_read_some(boost::asio::null_buffers(), 
            boost::bind(&TcpClientSocket::HandleRead, this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else //û��������
    {
        Connect(); //��������
    }    
}

/*------------------------------------------------------------------------------
 *��  ��: �ж��Ƿ��Ѿ����϶Զ˵�ַ
 *��  ��:
 *����ֵ: �Ƿ��Ѿ����϶Զ˵�ַ
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
bool TcpClientSocket::IsConnected() const
{
    return connected_;
}

/*------------------------------------------------------------------------------
 *��  ��: ���ӶԶ��յ����ݺ�Ļص�����
 *��  ��: [in]error �������
          [in]bytes_transferred �յ��������ֽ���
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocket::HandleRead(const boost::system::error_code& error, 
        uint64 /*bytes_transferred*/)
{
    auto handler = receive_handler(); //�������ݺ�Ļص�����
    if (handler)
    {
        size_t available = socket_.available(); //���Զ�ȡ���ֽ���
        receive_buffer_.resize(available);
        socket_.read_some(boost::asio::buffer(receive_buffer_)); //ͬ����ȡ����
        handler(error, (available > 0 ? &receive_buffer_[0] : nullptr), available);
    }
 
    //�����ӶԶ˽�������
    socket_.async_read_some(boost::asio::null_buffers(), 
        boost::bind(&TcpClientSocket::HandleRead, this, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * ��  ��: �رմ��׽���
 * ��  ��:
 * ����ֵ: �ر��׽����Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool TcpClientSocket::Close()
{
    socket_.close(); //ͬ���ر��׽���

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: TcpClientSocket��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
TcpClientSocket::~TcpClientSocket()
{
    Close(); //�ر��׽���
}

/*------------------------------------------------------------------------------
 *��  ��: ��Զ˷�������
 *��  ��: [in]data Ҫ���͵����ݵ�ָ��
          [in]length Ҫ�������ݵ��ֽ���
 *����ֵ: �����Ƿ�ɹ� 
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
bool TcpClientSocket::Send(const char* data, uint64 length)
{
    io_service().post(boost::bind(&TcpClientSocket::DoSend, this, data, length));    

    return true;
}

/*------------------------------------------------------------------------------
 *��  ��: ��Զ˷�������
 *��  ��: [in]data Ҫ���͵����ݵ�ָ��
          [in]length Ҫ�������ݵ��ֽ���
 *����ֵ: 
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocket::DoSend(const char* data, uint64 length)
{
    boost::asio::async_write(socket_, boost::asio::buffer(data, length), 
        boost::bind(&TcpClientSocket::HandleSent, this, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 *��  ��: �������Ѿ����ͳɹ���Ļص�����
 *��  ��: [in]error �������
 *����ֵ: [in]bytes_transferred ���ݳɹ����͵��ֽ���
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
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
 * ��  ��: ��ȡ�׽��ֱ��˵�ַ
 * ��  ��:
 * ����ֵ: �׽��ֱ��˵�ַ
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
EndPoint TcpClientSocket::local_address() const
{
    return to_endpoint(socket_.local_endpoint());
}

/*------------------------------------------------------------------------------
 *��  ��: UdpClientSocket���캯��
 *��  ��: [in]ios IO����
          [in]remote_address ��������ַ
          [in]handler ���յ��Զ����ݺ�Ļص�����
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
UdpClientSocket::UdpClientSocket(boost::asio::io_service& ios,
    const EndPoint& remote_address,
    const ReceiveHandler& handler) : 
    ClientSocket(ios, remote_address, handler),
    socket_(ios, boost::asio::ip::udp::endpoint(
        boost::asio::ip::udp::v4(), 0))
{
    endpoint_ = to_udp_endpoint(remote_address);

    //�ӷ�������������
    socket_.async_receive_from(boost::asio::null_buffers(), endpoint_, 
        boost::bind(&UdpClientSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * ��  ��: UdpClientSocket�����������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
UdpClientSocket::~UdpClientSocket()
{
    Close(); //�رմ��׽���
}

/*------------------------------------------------------------------------------
 *��  ��: ��Զ˷�������
 *��  ��: [in]data Ҫ�������ݵ�ָ��
          [in]length Ҫ�������ݵ��ֽ���
 *����ֵ: ���������Ƿ�ɹ�
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
bool UdpClientSocket::Send(const char* data, uint64 length)
{
    io_service().post(boost::bind(&UdpClientSocket::DoSend, this, data, length));

    return true;
}

/*------------------------------------------------------------------------------
 *��  ��: ��Զ˷�������
 *��  ��: [in]data Ҫ�������ݵ�ָ��
          [in]length Ҫ�������ݵ��ֽ���
 *����ֵ: 
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
void UdpClientSocket::DoSend(const char* data, uint64 length)
{    
    socket_.async_send_to(boost::asio::buffer(data, length), endpoint_,
        boost::bind(&UdpClientSocket::HandleSendTo, this, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * ��  ��: �رմ��׽���
 * ��  ��:
 * ����ֵ: �ر��׽����Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool UdpClientSocket::Close()
{
    socket_.close(); //�ر��׽���

    return true;
}

/*------------------------------------------------------------------------------
 *��  ��: �����ݷ�����ɺ�Ļص�����
 *��  ��: [in]error �������
          [in]bytes_transferred ʵ�ʷ��͵��ֽ���
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
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
 *��  ��: ���ӶԶ˽��յ����ݺ�Ļص�����
 *��  ��: [in]error �������
          [in]bytes_transferred ���յ��������ֽ���
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
void UdpClientSocket::HandleReceiveFrom(const boost::system::error_code& error, uint64 /*bytes_transferred*/)
{
    auto handler = receive_handler(); //���յ����ݺ�Ļص�����
    if (handler)
    {
        size_t available = socket_.available(); //���Զ�ȡ���ֽ���
        receive_buffer_.resize(available); 
        socket_.receive(boost::asio::buffer(receive_buffer_)); //��ȡ����
        handler(error, (available > 0 ? &receive_buffer_[0] : nullptr), available);
    }
    
    //������������
    socket_.async_receive_from(boost::asio::null_buffers(), endpoint_, 
        boost::bind(&UdpClientSocket::HandleReceiveFrom, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�����׽��ֵ�ַ
 * ��  ��:
 * ����ֵ: �����׽��ֵ�ַ
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
EndPoint UdpClientSocket::local_address() const
{
    return to_endpoint(socket_.local_endpoint());
}

}
