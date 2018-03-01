/*##############################################################################
 * �ļ���   : client_socket_ex.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.28
 * �ļ����� : TcpClientSocketEx��������ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
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
 * ��  ��: ����ʵ��TCP�ͻ��˶�����������
 * ��  ��: rosan
 * ʱ  ��: 2013.11.28
 ******************************************************************************/
class TcpClientSocketEx
{
public: 
    // ���ӷ������յ����ݺ�Ļص�����
    typedef boost::function<void(const error_code& error, const char* data, uint64 length)> ReceiveHandler;

    // �������Ѿ����͵���������Ļص�����
    typedef boost::function<void(const error_code& error, uint64 bytes_transferred)> SentHandler;

	// ���Զ������Ϻ�Ļص�����
	typedef boost::function<void()> ConnectedHandler;

	// ���Զ�ʧȥ���Ӻ�Ļص�����
	typedef boost::function<void()> DisconnectedHandler;
 
    TcpClientSocketEx(boost::asio::io_service& ios, const EndPoint& remote_address);
    ~TcpClientSocketEx();

    bool Send(const char* data, uint64 length);  // �������ݵ��Զ�
    void Close();  // �ر�socket

    EndPoint local_address() const;  // ��ȡ���˵�ַ
	EndPoint remote_address() const;  // ��ȡ�Զ˵�ַ
    
	ReceiveHandler receive_handler() const;  // ��ȡ�������ݻص�����
	void set_receive_handler(const ReceiveHandler& handler);  // ���ý������ݻص�����

	SentHandler sent_handler() const;  // ��ȡ���ݷ�����ɻص�����
	void set_sent_handler(const SentHandler& handler);  // �������ݷ�����ɻص�����

	ConnectedHandler connected_handler() const;
	void set_connected_handler(const ConnectedHandler& handler);

	DisconnectedHandler disconnected_handler() const;
	void set_disconnected_handler(const DisconnectedHandler& handler);

	boost::asio::io_service& io_service() const;  // ��ȡIO�������

	bool IsConnected() const;  // �Ƿ����϶Զ�

private:
	void Connect();  // ���ӶԶ˵�ַ
	void DoSend(const char* data, uint64 length);  // ��Զ˷�������
	void HandleConnect(const boost::system::error_code& error);  // ���϶Զ˺�Ļص�����
	void HandleRead(const boost::system::error_code& error, uint64 bytes_transferred);  // ���ӶԶ��յ����ݺ�Ļص�����
	void HandleSent(const boost::system::error_code& error, uint64 bytes_transferred);  // �����ݷ�����ɺ�Ļص�����
	void OnTimer();  // ���������Ķ�ʱ��

private:
	boost::asio::io_service& io_service_; // IO�������
    EndPoint remote_address_; // ��������ַ
	boost::asio::ip::tcp::socket socket_; //socket����
    ReceiveHandler receive_handler_; // �ӶԶ��յ����ݺ�Ļص�����
    SentHandler sent_handler_; // �����ݷ�����ɺ�Ļص�����
	ConnectedHandler connected_handler_;
	DisconnectedHandler disconnected_handler_;
	bool connected_; //��ʶ�Զ��Ƿ�������
	std::vector<char> receive_buffer_; //�������ݻ�����
	Timer timer_;  // ���������Ķ�ʱ��
};

}  // namespace BroadCache

#endif  // HEADER_CLIENT_SOCKET_EX
