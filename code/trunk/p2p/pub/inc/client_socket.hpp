/*#############################################################################
 * �ļ���   : client_socket.hpp
 * ������   : rosan
 * ����ʱ�� : 2013��08��22��
 * �ļ����� : ClientSocket TcpClientSocket UdpClientSocket������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * �������ͻ����׽��ֻ���
 * ���ߣ�rosan
 * ʱ�䣺2013/08/22
 *****************************************************************************/
class ClientSocket
{
public: 
    // ���ӷ������յ����ݺ�Ļص�����
    typedef boost::function<void(const error_code& error, const char* data, uint64 length)> ReceiveHandler;

    // �������Ѿ����͵���������Ļص�����
    typedef boost::function<void(const error_code& error, uint64 bytes_transferred)> SentHandler;
 
    ClientSocket(boost::asio::io_service& ios, const EndPoint& remote_address,
            const ReceiveHandler& handler = ReceiveHandler());

    virtual ~ClientSocket();

    virtual bool Send(const char* data, uint64 length) = 0;  // �������ݵ��Զ�
    virtual bool Close() = 0;  // �ر�socket
    virtual EndPoint local_address() const = 0;  // ��ȡ���˵�ַ
    
	ReceiveHandler receive_handler() const;  // ��ȡ�������ݻص�����
	void set_receive_handler(const ReceiveHandler& handler);  // ���ý������ݻص�����

	SentHandler sent_handler() const;  // ��ȡ���ݷ�����ɻص�����
	void set_sent_handler(const SentHandler& handler);  // �������ݷ�����ɻص�����

	boost::asio::io_service& io_service() const;  // ��ȡIO�������

    EndPoint remote_address() const;  // ��ȡ�Զ˵�ַ

private:
    EndPoint remote_address_; // ��������ַ
    ReceiveHandler receive_handler_; // �ӶԶ��յ����ݺ�Ļص�����
    SentHandler sent_handler_; // �����ݷ�����ɺ�Ļص�����
    boost::asio::io_service& io_service_; // IO�������
};

/******************************************************************************
 * ������TCP�ͻ���socket
 * ���ߣ�rosan
 * ʱ�䣺2013/08/22
 *****************************************************************************/
class TcpClientSocket : public ClientSocket
{
public:
    TcpClientSocket(boost::asio::io_service& ios, const EndPoint& remote_address,
        const ReceiveHandler& handler = ReceiveHandler());

    virtual ~TcpClientSocket();
    
    virtual bool Send(const char* data, uint64 length) override;  // �������ݵ��Զ�
    virtual bool Close() override;  // �ر�socket
    virtual EndPoint local_address() const override;  // ��ȡ���˵�ַ

    bool IsConnected() const;  // �Ƿ����϶Զ�

private:
    void Connect();  // ���ӶԶ˵�ַ
    void DoSend(const char* data, uint64 length);  // ��Զ˷�������
    void HandleConnect(const boost::system::error_code& error);  // ���϶Զ˺�Ļص�����
    void HandleRead(const boost::system::error_code& error, uint64 bytes_transferred);  // ���ӶԶ��յ����ݺ�Ļص�����
    void HandleSent(const boost::system::error_code& error, uint64 bytes_transferred);  // �����ݷ�����ɺ�Ļص�����

private:
    bool connected_; //��ʶ�Զ��Ƿ�������
    std::vector<char> receive_buffer_; //�������ݻ�����
    boost::asio::ip::tcp::socket socket_; //socket����
};

/******************************************************************************
 * ������UDP�ͻ���socket
 * ���ߣ�rosan
 * ʱ�䣺2013/08/22
 *****************************************************************************/
class UdpClientSocket : public ClientSocket
{
public:
    UdpClientSocket(boost::asio::io_service& ios, const EndPoint& remote_address,
        const ReceiveHandler& handler = ReceiveHandler());

    virtual ~UdpClientSocket();
 
    virtual bool Send(const char* data, uint64 length) override;  // �������ݵ�������
    virtual bool Close() override;  // �ر�socket
    virtual EndPoint local_address() const override;   // ��ȡ���ص�ַ

private:
    void DoSend(const char* data, uint64 length);  // �������ݵ��Զ�
    void HandleReceiveFrom(const boost::system::error_code& error, uint64 bytes_transferred);  // �ӷ��������յ����ݺ�Ļص�����
    void HandleSendTo(const boost::system::error_code& error, uint64 bytes_transferred);  // ���ݷ�����ɺ�Ļص�����

private:
    std::vector<char> receive_buffer_; //�������ݻ�����
    boost::asio::ip::udp::socket socket_; //socket����
    boost::asio::ip::udp::endpoint endpoint_; //�Զ˵�ַ
};

}

#endif  // HEADER_CLIENT_SOCKET
