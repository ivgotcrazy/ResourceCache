/**************************************************************************
 *�ļ���  : socket_connection.hpp
 *������  : teck_zhou
 *����ʱ��: 2013.10.08
 *�ļ�����: SocketConnection���������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 *************************************************************************/

#ifndef BROADCACHE_SOCKET_CONNECTION
#define BROADCACHE_SOCKET_CONNECTION

#include <atomic>
#include <queue>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "connection.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: socket���ӻ��࣬�����ĵ��շ�
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
class SocketConnection : public boost::noncopyable, 
	                     public boost::enable_shared_from_this<SocketConnection>
{
public:
	typedef boost::function<void(ConnectionError, const char*, uint64)> ReceiveHandler;
	typedef boost::function<void(ConnectionError, uint64)> SendHandler;
	typedef boost::function<void(ConnectionError)> ConnectHandler;

	SocketConnection(const EndPoint& remote); // ��������
	SocketConnection(const SocketServerSP& server, const EndPoint& remote); // ��������
	virtual ~SocketConnection();

	// ����������Ҫ���ô˽ӿ���ʾ�������ӣ���ǰ�ݲ�֧���ظ�����
	void Connect();
	void SendData(const SendBuffer& send_buf);
	void Close();
	
	void set_connect_handler(ConnectHandler handler) { connect_handler_ = handler; }
	void set_recv_handler(ReceiveHandler handler) { recv_handler_ = handler; }
	void set_send_handler(SendHandler hanlder) { send_handler_ = hanlder; }

	bool is_connected() const { return is_connected_; }
	ConnectionID connection_id() const { return conn_id_; }
	ConnectionType connection_type() const { return conn_type_; }

	bool operator==(const SocketConnection& conn) const { return conn.conn_id_ == conn_id_; }

protected:
	void StartSendWithLock();
	void OnConnect(const error_code& ec);
	void OnReceive(const error_code& ec, const char* buf, uint64 bytes_transferred);
	void OnSend(const error_code& ec, uint64 bytes_transferred);

protected:
	ConnectionID conn_id_;
	ConnectionType conn_type_;

private:
	virtual void DoConnect() = 0;
	virtual void DoSendData(const SendBuffer& send_buf) = 0;
	virtual void DoClose() = 0;

private:
	// ��������socket����������������������
	SocketServerWP socket_server_; 

	ConnectHandler connect_handler_;
	ReceiveHandler recv_handler_;
	SendHandler send_handler_;

	std::queue<SendBuffer> send_buffer_queue_;
	boost::mutex send_queue_mutex_;
	
	// �����Ƿ��Ѿ��������˱�־���������Ƿ���Խ��б����շ�
	bool is_connected_;

	// ǰһ�������Ƿ��Ѿ�������ϲ��յ���Ӧ�첽��Ӧ(true:�ǣ�false:��)
	// �˱�־������������������б��ķ���
	bool is_ready_;
};

/******************************************************************************
 * ����: TCP���͵�socket����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
class TcpSocketConnection : public SocketConnection
{
public:
	TcpSocketConnection(io_service& ios, const EndPoint& remote); // ͬ����������
    TcpSocketConnection(io_service& ios, const EndPoint& remote, ConnectHandler handler); // �첽��������
	TcpSocketConnection(const TcpSocketServerSP& server, const TcpSocketSP& socket, const EndPoint& remote); // ��������
	~TcpSocketConnection();

private:
	virtual void DoConnect();
	virtual void DoSendData(const SendBuffer& send_buf);
	virtual void DoClose();

private:
	void StartReceive();
	void HandleConnect(const error_code& ec);
	void HandleReceive(const error_code& ec, uint64 /*bytes_transferred*/);
	void HandleSend(const error_code& ec, uint64 bytes_transferred);
	void CloseSocket();

private:
	TcpSocketSP socket_;
	ConnectHandler connect_handler_;
	std::vector<char> receive_buffer_;
};

/******************************************************************************
 * ����: UDP���͵�socket����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
class UdpSocketConnection : public SocketConnection
{
public:
	UdpSocketConnection(io_service& ios, const EndPoint& remote); // ��������
	UdpSocketConnection(const UdpSocketServerSP& server, const UdpSocketSP& socket, const EndPoint& remote); // ��������
	~UdpSocketConnection();

	// UDP�������ӵı����շ��ɷ�����socket����
	void OnReceiveData(const error_code& ec, const char* buf, uint64 len);
	void OnSendData(const error_code& ec, uint64 bytes_transferred);

private:
	virtual void DoConnect();
	virtual void DoSendData(const SendBuffer& send_buf);
	virtual void DoClose();

private:
	void StartReceive();
	void HandleReceive(const error_code& ec, uint64 /*bytes_transferred*/);
	void HandleSend(const error_code& ec, uint64 bytes_transferred);
	void CloseSocket();

private:
	UdpSocketSP socket_;
	udp_endpoint sender_endpoint_;
	std::vector<char> receive_buffer_;
	UdpSocketServerSP udp_server_;
};

/*-----------------------------------------------------------------------------
 * ��  ��: ��ӡ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
template<class Stream>
inline Stream& operator<<(Stream& stream, const SocketConnection& conn)
{
	stream << conn.connection_id();
	return stream;
}

}

#endif
