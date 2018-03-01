/*------------------------------------------------------------------------------
 *�ļ���  : server_socket.hpp
 *������  : rosan
 *����ʱ��: 2013.08.22
 *�ļ�����: ServerSocket TcpServerSocket UdpServerSocket
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 *��  ��: ������socket����
 *��  ��: rosan
 *ʱ  ��: 2013.08.22
 -----------------------------------------------------------------------------*/ 
class ServerSocket
{
public:
    // �ӶԶ˽��յ����ݺ�Ļص�����
    typedef boost::function<void(const EndPoint&, const error_code&, const char*, uint64)> ReceiveHandler;

    // ���ݷ�����ɺ�Ļص�����
    typedef boost::function<void(const EndPoint&, const error_code&, uint64)> SentHandler;

    ServerSocket(unsigned short local_port, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    ServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    virtual ~ServerSocket();
    
    virtual bool Send(const EndPoint& remote_address, const char* data, uint64 length) = 0;  // �������ݵ��Զ�

    ReceiveHandler receive_handler() const;  // ��ȡ���ݽ��ջص�����
    void set_receive_handler(const ReceiveHandler& handler);  // �������ݽ��ջص�����

    SentHandler sent_handler() const;  // ��ȡ���ݷ�����ɻص�����
    void set_sent_handler(const SentHandler& handler);  // �������ݷ�����ɻص�����

	EndPoint local_address() const;  // ��ȡ���˵�ַ

	boost::asio::io_service& io_service() const;  // ��ȡIO�������
    
private:
    EndPoint local_address_;  // ���˵�ַ
    boost::asio::io_service& io_service_;  // IO�������
    ReceiveHandler receive_handler_;  // ���ӶԶ��յ����ݺ�Ļص�����
    SentHandler sent_handler_;  // ���ݷ�����ɺ�Ļص�����
};

/*------------------------------------------------------------------------------
 *��  ��: TCP������socket
 *��  ��: rosan
 *ʱ  ��: 2013.08.22
 -----------------------------------------------------------------------------*/ 
class TcpServerSocket : public ServerSocket
{
public:
    typedef boost::function<void(const EndPoint&)> ConnectedHandler;  // �Զ������ϵĻص�����
    typedef boost::function<void(const EndPoint&)> DisconnectedHandler;  // �Զ˶Ͽ����Ӻ�Ļص�����

    TcpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    TcpServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    virtual ~TcpServerSocket();
    
    virtual bool Send(const EndPoint& remote_address, const char* data, uint64 length) override;  // �������ݵ��ͻ��˽ӿ�
    
    ConnectedHandler connected_handler() const;  // ��ȡ�Զ������ϵĻص�����
    void set_connected_handler(const ConnectedHandler& handler);  // ���öԶ������ϵĻص�����

    DisconnectedHandler disconnected_handler() const;  // ��ȡ�Զ˶Ͽ����Ӻ�Ļص�����
    void set_disconnected_handler(const DisconnectedHandler& handler);  // ���öԶ˶Ͽ����Ӻ�Ļص�����

	boost::unordered_set<EndPoint> GetClients() const;  // ��ȡ�Զ˵�ַ�б�

private:
    struct Session;

private:
	void StartAccept();  // �ȴ����Կͻ��˵�����

    void DoSend(boost::asio::ip::tcp::socket* socket, const EndPoint& remote_address, 
            const char* data, uint64 length);  // �������ݵ��Զ�
    
    bool RemoveSession(const EndPoint& endpoint);  // ɾ��һ���ͻ��˻Ự�����ӣ�
    
    void HandleAccept(Session* new_session, const boost::system::error_code& error);  // �������Կͻ��˵�����

    //�������Կͻ��˵����ݵĻص�����
    void HandleRead(const EndPoint& endpoint, const boost::system::error_code& error, uint64 bytes_transferred);

    //�����ݷ��͵��ͻ��˺�Ļص�����
    void HandleWrite(const EndPoint& remote_address, const boost::system::error_code& error, uint64 bytes_transferred);
 
private:
    struct Session  // �ͻ��˻Ự
    {
        explicit Session(boost::asio::io_service& ios);

        boost::asio::ip::tcp::socket socket;  // ����ͻ������ӵ�socket
        std::vector<char> receive_buffer;  // �������Կͻ��˵����ݵĻ�����
    }; 

private:
    ConnectedHandler connected_handler_;  // �ͻ��������Ϸ�������Ļص�����
    DisconnectedHandler disconnected_handler_;  // ������ʧȥ���Ӻ�Ļص�����
    boost::asio::ip::tcp::acceptor acceptor_;  // �ͻ������ӽ�����
    boost::unordered_map<EndPoint, Session*> sessions_;  // �ͻ��˻Ự
};

/*------------------------------------------------------------------------------
 *��  ��: UDP������socket
 *��  ��: rosan
 *ʱ  ��: 2013.08.22
 -----------------------------------------------------------------------------*/ 
class UdpServerSocket : public ServerSocket
{
public:
    UdpServerSocket(unsigned short local_port, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());
    UdpServerSocket(const EndPoint& local_address, boost::asio::io_service& ios,
        const ReceiveHandler& handler = ReceiveHandler());

    virtual ~UdpServerSocket();
 
    virtual bool Send(const EndPoint& remote_address, const char* data, uint64 length) override;  // �������ݵ��Զ�

private:
    void DoSend(const EndPoint& remote_address, const char* data, uint64 length);  // �������ݵ��Զ�

    // ���յ��Զ����ݺ�Ļص�����
    void HandleReceiveFrom(const boost::system::error_code& error, uint64 bytes_transferred);

    // �����ݷ�����ɺ�Ļص�����
    void HandleSendTo(const EndPoint& remote_address, const boost::system::error_code& error, 
        uint64 bytes_transferred);

private:
    boost::asio::ip::udp::socket socket_; // socket
    boost::asio::ip::udp::endpoint sender_endpoint_; //���һ���������ݵ�UDP��������UDP�ͻ��˵�ַ
    std::vector<char> receive_buffer_; // �������ݵĻ�����
};

}

#endif  // HEADER_SERVER_SOCKET
