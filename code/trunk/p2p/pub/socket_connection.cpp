/*#############################################################################
 * �ļ���   : socket_connection.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��08��
 * �ļ����� : SocketConnection�����ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "socket_connection.hpp"
#include "socket_server.hpp"
#include <glog/logging.h>
#include <boost/bind.hpp>
#include "bc_assert.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: remote Զ�˵�ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketConnection::SocketConnection(const EndPoint& remote)
	: is_connected_(false), is_ready_(true)
{
	conn_id_.remote = remote;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: server ����socket������
 *         remote Զ�˵�ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketConnection::SocketConnection(const SocketServerSP& server, const EndPoint& remote)
	: socket_server_(server), is_connected_(false), is_ready_(true)
{
	conn_id_.remote = remote;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: server ����socket������
 *         remote Զ�˵�ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketConnection::~SocketConnection()
{
	LOG(INFO) << ">>> Destructing socket connection | " << conn_id_;

	is_connected_ = false;

	// �ͷ�δ���ü����͵������ڴ棬������֪ͨ�û�
	boost::mutex::scoped_lock lock(send_queue_mutex_);
	while (!send_buffer_queue_.empty())
	{
		SendBuffer& send_buf = send_buffer_queue_.front();
		if (send_buf.free_func)
		{
			send_buf.free_func(send_buf.buf);
		}
		send_buffer_queue_.pop();
	}

	// ����Ǳ���������֪ͨ������ɾ����Ӧ����
	if (conn_type_ == CONN_PASSIVE)
	{
		SocketServerSP server = socket_server_.lock();
		if (server)
		{
			server->RemoveSocketConnection(conn_id_);
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketConnection::Connect()
{
	// �������෽��ִ�����Ӷ���
	DoConnect();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ر�����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketConnection::Close()
{
	LOG(INFO) << "### Close socket connection | " << connection_id();

	DoClose(); // �������෽��ִ�йرն���
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: [in] send_buf ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketConnection::SendData(const SendBuffer& send_buf)
{
	BC_ASSERT(send_buf.cursor < send_buf.len);

	// ����UDP�������ӣ�����ֱ���Ӹ�������������ʹ�����ӵĻ���
	if ((conn_type_ == CONN_PASSIVE) && (conn_id_.protocol == CONN_PROTOCOL_UDP))
	{
		DoSendData(send_buf);
		return;
	}

	// �Ƚ������͵�buffer�������
	boost::mutex::scoped_lock lock(send_queue_mutex_);
	send_buffer_queue_.push(send_buf);
	
	// ��������Ѿ��������Ҵ��ڿ���״̬�������Ϸ��ͱ��ģ�
	// ������Ҫ�ȵ����ӽ����ɹ�����ǰ���buffer��������ټ������ͱ���
	if (is_connected_ && is_ready_)
	{
		DoSendData(send_buffer_queue_.front());
		is_ready_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketConnection::StartSendWithLock()
{
	boost::mutex::scoped_lock lock(send_queue_mutex_);

	if (!send_buffer_queue_.empty() && is_ready_)
	{
		DoSendData(send_buffer_queue_.front());
		is_ready_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ر�����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketConnection::OnConnect(const error_code& ec)
{
	is_connected_ = true;

	if (connect_handler_)
	{
		connect_handler_(ToConnErr(ec));
	}

	// ��ʼ���ͱ���
	StartSendWithLock();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ϲ���մ���
 * ��  ��: [in] ec ������
 *         [in] buf ��������
 *         [in] bytes_transferred �������ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketConnection::OnReceive(const error_code& ec, const char* buf, uint64 bytes_transferred)
{
	if (ec)
	{
		LOG(WARNING) << "Connection error(" << ec.message() << ") | " << *this;
	}

	ConnectionError error = ToConnErr(ec);

	// ���ڽ��ձ��ĳ���Ϊ0��Ҫ���⴦��
	if (bytes_transferred == 0)
	{
		if (conn_id_.protocol == CONN_PROTOCOL_TCP)
		{
			LOG(INFO) << "Peer closed connection | " << *this;
			error = CONN_ERR_REMOTE_CLOSED;
		}
	}

	// �ص��û����Ĵ�����
	if (recv_handler_)
	{
		recv_handler_(error, buf, bytes_transferred);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ϲ㷢�ʹ���
 * ��  ��: [in] ec ������
 *         [in] bytes_transferred �����ֽ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketConnection::OnSend(const error_code& ec, uint64 bytes_transferred)
{
	// ����UDP�������ӣ����ķ�����socket���������𣬴˴������ֱ��ת�����û�
	if ((conn_type_ == CONN_PASSIVE) && (conn_id_.protocol == CONN_PROTOCOL_UDP))
	{
		if(send_handler_)
		{
			send_handler_(ToConnErr(ec), bytes_transferred);
		}
		return;
	}

	boost::mutex::scoped_lock lock(send_queue_mutex_);

	is_ready_ = true; // ���Խ�����һ�α��ķ���

	// ���������ޱ����յ�������Ӧ����������ȼ�¼����
	if (send_buffer_queue_.empty())
	{
		LOG(WARNING) << "None send buffer while received send callback";
		return;
	}

	SendBuffer& buffer = send_buffer_queue_.front();
	if (!ec) // ���ͳɹ�
	{
		buffer.cursor += bytes_transferred;
		
		if (buffer.cursor > buffer.len)
		{
			LOG(ERROR) << "*****************************************************";
			LOG(ERROR) << "cursor: " << buffer.cursor << " len: " << buffer.len;
			LOG(ERROR) << "*****************************************************";
		}

		BC_ASSERT(buffer.cursor <= buffer.len);
	}

	// ����ʧ�ܻ���һ��buffer�������
	if (ec || buffer.cursor == buffer.len)
	{
		if (buffer.send_buffer_handler) // �ص��û�����
		{
			buffer.send_buffer_handler(ToConnErr(ec));
		}

		if (buffer.free_func) // �ͷ��ڴ�
		{
			buffer.free_func(buffer.buf);
		}

		send_buffer_queue_.pop();
	}

	// �����л����������������
	if (!send_buffer_queue_.empty())
	{
		SendBuffer& next_buffer = send_buffer_queue_.front();
		DoSendData(next_buffer);

		is_ready_ = false;
	}
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: TCP�������ӹ��캯��
 * ��  ��: ios io_service
 *         remote Զ�˵�ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
TcpSocketConnection::TcpSocketConnection(io_service& ios, const EndPoint& remote)
	: SocketConnection(remote)
	, socket_(new tcp_socket(ios, tcp_endpoint(boost::asio::ip::tcp::v4(), 0)))
{
	conn_type_ = CONN_ACTIVE;
	conn_id_.protocol = CONN_PROTOCOL_TCP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * ��  ��: TCP�������ӹ��캯��
 * ��  ��: server ����socket������
 *         socket ����socket
 *         remote Զ�˵�ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
TcpSocketConnection::TcpSocketConnection(const TcpSocketServerSP& server, 
	const TcpSocketSP& socket, const EndPoint& remote)
	: SocketConnection(server, remote)
	, socket_(socket)
{
	conn_type_ = CONN_PASSIVE;
	conn_id_.protocol = CONN_PROTOCOL_TCP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
TcpSocketConnection::~TcpSocketConnection()
{
	LOG(INFO) << ">>> Destructing TcpSocketConnection | " << connection_id();  

	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ӵ���
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::DoConnect()
{
	if (conn_type_ == CONN_PASSIVE)
	{
		// TCP�������Ӳ���Ҫ��ȥconnect�ˣ�ֱ��֪ͨ�ϲ�
		OnConnect(boost::system::errc::make_error_code(boost::system::errc::success));

		// ��ʼ��������
		StartReceive();
	}
	else
	{
		socket_->async_connect(
			to_tcp_endpoint(conn_id_.remote), 
			boost::bind(&TcpSocketConnection::HandleConnect, 
			boost::dynamic_pointer_cast<TcpSocketConnection>(shared_from_this()),
			boost::asio::placeholders::error
			)
		);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ͱ���
 * ��  ��: [in] send_buf ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::DoSendData(const SendBuffer& send_buf)
{
	boost::asio::async_write(
		*socket_, 
		boost::asio::buffer(send_buf.buf + send_buf.cursor, send_buf.len - send_buf.cursor), 
		boost::bind(&TcpSocketConnection::HandleSend, 
					boost::dynamic_pointer_cast<TcpSocketConnection>(shared_from_this()), 
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ȥʹ��socket�ϵĶ�/д���������ر�socket
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::DoClose()
{
	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����CloseSocket�Ǵ��麯������Ҫ�����ṩһ��������������������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::CloseSocket()
{
	error_code ec;
	socket_->close(ec);
	if (ec)
	{
		LOG(WARNING) << "Fail to close socket | " << ec.message();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������ӽ����ɹ������Ͻ�����ձ���״̬
 * ��  ��: [in] ec ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::HandleConnect(const error_code& ec)
{
	OnConnect(ec);

	//TODO: �������Կ����Ż�Ϊ���Զ������
	if (ec)
	{
		LOG(WARNING) << "Failed to connect | " << *this;
	}
	else
	{
		LOG(INFO) << "Success to connect | " << *this;	
	}

	// ��ʼ��������
	StartReceive();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ձ���״̬
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::StartReceive()
{
	// �˽ӿڶ������ݾͻ᷵��
	socket_->async_read_some(
		boost::asio::null_buffers(), 
		boost::bind(&TcpSocketConnection::HandleReceive, 
		            boost::dynamic_pointer_cast<TcpSocketConnection>(shared_from_this()), 
					boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ľ��մ���
 * ��  ��: [in] ec ������
 *         [in] uint64 ���ձ��ĳ��ȣ�δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::HandleReceive(const error_code& ec, uint64)
{
	if (ec)
	{
		LOG(WARNING) << "Receive data error(" << ec << ") | " << *this;
		OnReceive(ec, 0, 0);
		return;
	}

	uint64 available = socket_->available();
	receive_buffer_.resize(available);
	socket_->read_some(boost::asio::buffer(receive_buffer_));

	// ֪ͨ�ϲ�
	OnReceive(ec, receive_buffer_.data(), receive_buffer_.size());

	// ����TCP��˵������0�ֽ���Ϊ�����Ѿ��رգ���Ӧ���ټ�������
	if (receive_buffer_.size() != 0)
	{
		StartReceive();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ķ��ʹ���
 * ��  ��: [in] ec ������
 *         [in] bytes_transferred ���ͱ��ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketConnection::HandleSend(const error_code& ec, uint64 bytes_transferred)
{
	// ֪ͨ�ϲ�
	OnSend(ec, bytes_transferred);
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: UDP�������ӹ��캯��
 * ��  ��: ios io_service
 *         remote Զ�˵�ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
UdpSocketConnection::UdpSocketConnection(io_service& ios, const EndPoint& remote)
	: SocketConnection(remote)
	, socket_(new udp_socket(ios, udp_endpoint(boost::asio::ip::udp::v4(), 0)))
{
	conn_type_ = CONN_ACTIVE;
	conn_id_.protocol = CONN_PROTOCOL_UDP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * ��  ��: UDP�������ӹ��캯��
 * ��  ��: [in] server ����socket������
 *         [in] socket ����socket
 *         [in] remote Զ�˵�ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
UdpSocketConnection::UdpSocketConnection(const UdpSocketServerSP& server, 
	const UdpSocketSP& socket, const EndPoint& remote)
	: SocketConnection(server, remote)
	, socket_(socket)
	, udp_server_(server)
{
	conn_type_ = CONN_PASSIVE;
	conn_id_.protocol = CONN_PROTOCOL_UDP;
	conn_id_.local = to_endpoint(socket_->local_endpoint());
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
UdpSocketConnection::~UdpSocketConnection()
{
	LOG(INFO) << ">>> Destructing UdpSocketConnection | " << connection_id();

	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ӵ���
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��02��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::DoConnect()
{
	// ����UDP��˵��û�����ӹ��̣���������Ϊ�Ѿ�����OK
	OnConnect(boost::system::errc::make_error_code(boost::system::errc::success));

	// �����������ӣ�������ʼ���ձ���
	if (conn_type_ == CONN_ACTIVE)
	{
		StartReceive();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������ݴ���
 * ��  ��: [in] send_buf ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::DoSendData(const SendBuffer& send_buf)
{
	if (conn_type_ == CONN_PASSIVE)
	{
		SendToBuffer buffer(
			boost::dynamic_pointer_cast<UdpSocketConnection>(shared_from_this()), 
			send_buf, 
			conn_id_.remote
		);
		udp_server_->SendData(buffer);
	}
	else
	{
		// endpoint��������Ż�
		socket_->async_send_to(
			boost::asio::buffer(send_buf.buf + send_buf.cursor, send_buf.len - send_buf.cursor), 
			to_udp_endpoint(conn_id_.remote),
			boost::bind(&UdpSocketConnection::HandleSend, 
			            boost::dynamic_pointer_cast<UdpSocketConnection>(shared_from_this()), 
			            boost::asio::placeholders::error,
			            boost::asio::placeholders::bytes_transferred
			)
		);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ȥʹ��socket�ϵĶ�/д���������ر�socket
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::DoClose()
{
	CloseSocket();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����CloseSocket�Ǵ��麯������Ҫ�����ṩһ��������������������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::CloseSocket()
{
	if (connection_id().protocol == CONN_PROTOCOL_UDP && connection_type() == CONN_PASSIVE)
	{
		return;
	}
	
	error_code ec;
	socket_->close(ec);
	if (ec)
	{
		LOG(WARNING) << "Fail to close socket | " << ec.message();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ձ���
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::StartReceive()
{
	//�ӷ�������������
	socket_->async_receive_from(
		boost::asio::null_buffers(),
		sender_endpoint_, 
		boost::bind(&UdpSocketConnection::HandleReceive, 
		            boost::dynamic_pointer_cast<UdpSocketConnection>(shared_from_this()),
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * ��  ��: UDP�������ӱ��Ľ��մ�����
 * ��  ��: [in] ec ������
 *         [in] len
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::HandleReceive(const error_code& ec, uint64 len)
{
	if (!ec)
	{
		// ���Ľ��մ���
		uint64 available = socket_->available();
		receive_buffer_.resize(available);
		socket_->receive(boost::asio::buffer(receive_buffer_));
	}

	// ֪ͨ�ϲ�
	OnReceive(ec, receive_buffer_.data(), receive_buffer_.size());
	
	// ��������
	if (!ec)
	{
		StartReceive();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: UDP�������Ӵ����ķ���
 * ��  ��: [in] ec ������
 *         [in] bytes_transferred ���ķ��ʹ�С
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::HandleSend(const error_code& ec, uint64 bytes_transferred)
{
	// ֪ͨ�ϲ�
	OnSendData(ec, bytes_transferred);
}

/*-----------------------------------------------------------------------------
 * ��  ��: UDP�������ӣ������ɷ������ַ�����������ֻ��һ��ת��
 * ��  ��: [in] ec ������
 *         [in] buf ��������
 *         [in] len �������ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ��?����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::OnReceiveData(const error_code& ec, const char* buf, uint64 len)
{
	// ֪ͨ�ϲ�
	OnReceive(ec, buf, len);
}

/*-----------------------------------------------------------------------------
 * ��  ��: UDP�������ӣ����ķ��ͽ���ɷ�����֪ͨ����������ֻ��һ��ת��
 * ��  ��: [in] ec ������
 *         [in] bytes_transferred �������ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketConnection::OnSendData(const error_code& ec, uint64 bytes_transferred)
{
	// ֪ͨ�ϲ�
	OnSend(ec, bytes_transferred);
}

}
