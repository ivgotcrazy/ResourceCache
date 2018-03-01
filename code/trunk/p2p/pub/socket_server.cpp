/*#############################################################################
 * �ļ���   : socket_server.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��08��
 * �ļ����� : SocketServer���������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "socket_server.hpp"
#include "endpoint.hpp"
#include <glog/logging.h>
#include <boost/bind.hpp>
#include <boost/exception/all.hpp>
#include "socket_connection.hpp"
#include "bc_assert.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: local_address ������������ַ
 *         handler ���Ӵ����ص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketServer::SocketServer(const EndPoint& local_address, NewConnHandler handler)
	: local_address_(local_address), new_conn_handler_(handler)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketServer::~SocketServer()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool SocketServer::Start()
{
	LOG(INFO) << "Start socket server | " << local_address_;

	return Init();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƴ�����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketServer::RemoveSocketConnection(const ConnectionID& conn_id)
{
	boost::mutex::scoped_lock lock(conn_mutex_);

	ConnectionMapIter iter = conn_map_.find(conn_id);
	if (iter == conn_map_.end())
	{
		LOG(WARNING) << "Can't find socket connection";
		return;
	}
	else
	{
		LOG(INFO) << "Remove one socket connection | " << conn_id;
		conn_map_.erase(iter);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketConnectionSP SocketServer::FindSocketConnection(const ConnectionID& conn)
{
	boost::mutex::scoped_lock lock(conn_mutex_);

	ConnectionMapIter iter = conn_map_.find(conn);
	if (iter == conn_map_.end())
	{
		return SocketConnectionSP();
	}
	else
	{
		return iter->second.lock();
	}	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SocketServer::NewSocketConnection(const SocketConnectionSP& conn)
{
	LOG(INFO) << "Incomming a new socket connection | " << *(conn.get());

	// �����Ӳ����������
	{
		boost::mutex::scoped_lock lock(conn_mutex_);
		conn_map_.insert(ConnectionMap::value_type(conn->connection_id(), conn));
	}
	
	// ֪ͨ�½�����
	if (new_conn_handler_)
	{
		new_conn_handler_(conn);
	}
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: ios io_service
 *         local_address ���ص�ַ
 *         handler ��������֪ͨ�ӿ�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
TcpSocketServer::TcpSocketServer(io_service& ios, const EndPoint& local_address, NewConnHandler handler)
	: SocketServer(local_address, handler)
	, ios_(ios)
	, acceptor_(ios)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool TcpSocketServer::Init()
{	
	try
	{
		acceptor_.open(boost::asio::ip::tcp::v4());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true)); 
		acceptor_.bind(to_tcp_endpoint(local_address_));
		acceptor_.listen();
	}
	catch (std::exception& e)
	{
		LOG(ERROR) << "Fail to initialize socket server | " << e.what();
		return false;
	}
	
	// ��ʼ�����µ�����
	StartAccept();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼִ��TCP��Accept
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketServer::StartAccept()
{
	TcpSocketSP accept_socket(new tcp_socket(ios_));
	acceptor_.async_accept(
		*accept_socket,
		boost::bind(&TcpSocketServer::HandleAccept, 
		            shared_from_this(), 
					accept_socket, 
					boost::asio::placeholders::error
		)
	);
}

/*-----------------------------------------------------------------------------
 * ��  ��: Accept�ص�����
 * ��  ��: socket �²�����socket
 *         ec ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TcpSocketServer::HandleAccept(const TcpSocketSP& accept_socket, const error_code& ec)
{
	do 
	{
		if (ec)
		{
			LOG(WARNING) << "Accept error | " << local_address_;
			break;
		}

		TcpSocketConnectionSP conn;
		try
		{
			conn.reset(new TcpSocketConnection(shared_from_this(), 
				accept_socket, to_endpoint(accept_socket->remote_endpoint())));
		}
		catch (std::exception& e)
		{
			LOG(ERROR) << "Fail to create tcp socket connection | " << e.what();
			break;
		}

		NewSocketConnection(conn); // Accept�Ŀ϶���������

	}while(false);

	StartAccept();	// ����Accept
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: ios io_service
 *         local_address ���ص�ַ
 *         handler ��������֪ͨ�ӿ�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
UdpSocketServer::UdpSocketServer(io_service& ios, const EndPoint& local_address, NewConnHandler handler)
	: SocketServer(local_address, handler)
	, buffer_(UDP_MAX_PACKET_LEN)
	, socket_(new udp_socket(ios, to_udp_endpoint(local_address)))
	, is_busy_(false)
	, is_ready_(true)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����UDP��������˵��û��Accept���̣�ֱ����socket���ձ���
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool UdpSocketServer::Init()
{
	StartReceive();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ���ձ���
 * ��  ��: ec ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketServer::StartReceive()
{
	socket_->async_receive_from(
		boost::asio::buffer(buffer_, UDP_MAX_PACKET_LEN),
		sender_endpoint_,
		boost::bind(&UdpSocketServer::HandleReceive, 
		            shared_from_this(), 
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	); 
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����UDP������socket�����ӵı��Ľ��ն��ɷ�����ִ�У���Ҫ���������Ż�
 * ��  ��: ec ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketServer::HandleReceive(const error_code& ec, uint64 data_len)
{
	if (ec)
	{
		LOG(WARNING) << "Receive error(" <<ec.message() << ") | " 
			         << to_endpoint(sender_endpoint_);
	}
	else
	{
		// ���Ӵ���
		ConnectionID conn(local_address_, to_endpoint(sender_endpoint_), CONN_PROTOCOL_UDP);
		SocketConnectionSP socket_conn = FindSocketConnection(conn);
		if (!socket_conn)
		{
			socket_conn.reset(new UdpSocketConnection(
				shared_from_this(), socket_, to_endpoint(sender_endpoint_)));

			NewSocketConnection(socket_conn);
		}

		// ���Ľ��մ���
		//uint64 available = socket_->available();
		//receive_buffer_.resize(available);
		//socket_->receive(boost::asio::buffer(receive_buffer_));

		//DLOG(INFO) << "Received data size(" << receive_buffer_.size() 
		//	       << ") | " << *(socket_conn.get());

		
		DLOG(INFO) << "Received data size(" << data_len
			       << ") | " << *(socket_conn.get());

		// �ص����ӵı��Ĵ�����
		UdpSocketConnectionSP udp_socket_conn = 
			boost::dynamic_pointer_cast<UdpSocketConnection>(socket_conn);
		udp_socket_conn->OnReceiveData(ec, buffer_.data(), data_len);
	}

	// �������ձ���
	StartReceive();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ͱ���
 * ��  ��: ec ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketServer::DoSendData(const SendToBuffer& send_to_buf)
{
	// endpoint��������Ż�
	socket_->async_send_to(
		boost::asio::buffer(send_to_buf.send_buffer.buf + send_to_buf.send_buffer.cursor, 
		                    send_to_buf.send_buffer.len - send_to_buf.send_buffer.cursor), 
		to_udp_endpoint(send_to_buf.remote),
		boost::bind(&UdpSocketServer::HandleSend, 
		            shared_from_this(), 
		            boost::asio::placeholders::error,
		            boost::asio::placeholders::bytes_transferred
		)
	);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ķ��ʹ���
 * ��  ��: ec ������
 *         bytes_transferred �����ֽ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketServer::HandleSend(const error_code& ec, uint64 bytes_transferred)
{
	boost::mutex::scoped_lock lock(send_mutex_);
	SendToBuffer buffer = send_queue_.front();

	// ���Խ����´α��ķ���
	is_ready_ = true;

	if (!ec)
	{
		buffer.send_buffer.cursor += bytes_transferred;
		BC_ASSERT(buffer.send_buffer.cursor <= buffer.send_buffer.len);

		if (buffer.send_buffer.cursor == buffer.send_buffer.len)
		{
			buffer.connection->OnSendData(ec, buffer.send_buffer.cursor);
			send_queue_.pop();
		}
	}
	else
	{
		buffer.connection->OnSendData(ec, buffer.send_buffer.cursor);
		send_queue_.pop();
	}

	// �ͷŷ����ڴ�
	if (buffer.send_buffer.free_func)
	{
		buffer.send_buffer.free_func(buffer.send_buffer.buf);
	}

	BC_ASSERT(is_busy_);

	// ������Ͷ����л������ݣ����������
	if (!send_queue_.empty())
	{
		DoSendData(send_queue_.front());
		is_ready_ = false;
	}
	else
	{
		is_busy_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ķ��ͽӿ�
 * ��  ��: send_buf ��������
 *         bytes_transferred �����ֽ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UdpSocketServer::SendData(const SendToBuffer& buf)
{
	boost::mutex::scoped_lock lock(send_mutex_);
	send_queue_.push(buf);

	// ������д��ڿ���״̬����������������
	if (!is_busy_ && is_ready_)
	{
		DoSendData(send_queue_.front());
		is_ready_ = false;
		is_busy_ = true;
	}
}

}
