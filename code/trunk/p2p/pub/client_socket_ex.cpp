/*##############################################################################
 * �ļ���   : client_socket_ex.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.28
 * �ļ����� : TcpClientSocketEx��ʵ���ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "client_socket_ex.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *��  ��: TcpClientSocketEx���캯��
 *��  ��: [in]ios IO����
        : [in]remote_address �Զ˵�ַ
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::TcpClientSocketEx(boost::asio::io_service& ios,
    const EndPoint& remote_address) : 
	io_service_(ios),
	remote_address_(remote_address),
	socket_(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0)),
    connected_(false),
	timer_(ios, boost::bind(&TcpClientSocketEx::OnTimer, this), 1)
{
    timer_.Start();
}

/*------------------------------------------------------------------------------
 *��  ��: TcpClientSocketEx����������
 *��  ��:
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.20 
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::~TcpClientSocketEx()
{
	Close();
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
EndPoint TcpClientSocketEx::remote_address() const
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
TcpClientSocketEx::ReceiveHandler TcpClientSocketEx::receive_handler() const
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
void TcpClientSocketEx::set_receive_handler(const ReceiveHandler& handler)
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
boost::asio::io_service& TcpClientSocketEx::io_service() const
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
TcpClientSocketEx::SentHandler TcpClientSocketEx::sent_handler() const
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
void TcpClientSocketEx::set_sent_handler(const SentHandler& handler)
{
    sent_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص����� 
 *         �Զ������ϵĻص�����
 * ��  ��:
 * ����ֵ: �Զ������ϵĻص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::ConnectedHandler TcpClientSocketEx::connected_handler() const
{
	return connected_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         �Զ������ϵĻص�����
 * ��  ��: [in]handler �Զ������ϵĻص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_connected_handler(const ConnectedHandler& handler)
{
	connected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�ص����� 
 *         ���Զ�ʧȥ���Ӻ�Ļص�����
 * ��  ��:
 * ����ֵ: ���Զ�ʧȥ���Ӻ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::DisconnectedHandler TcpClientSocketEx::disconnected_handler() const
{
	return disconnected_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ûص�����
 *         ���Զ�ʧȥ���Ӻ�Ļص�����
 * ��  ��: [in]handler ���Զ�ʧȥ���Ӻ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_disconnected_handler(const DisconnectedHandler& handler)
{
	disconnected_handler_ = handler;
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
void TcpClientSocketEx::Connect()
{    
/*
    socket_.async_connect(to_tcp_endpoint(remote_address()), 
        boost::bind(&TcpClientSocketEx::HandleConnect, this,
            boost::asio::placeholders::error));
*/
    boost::system::error_code error; 
    socket_.connect(to_tcp_endpoint(remote_address()), error);
    
    if (!error)
    {
        connected_ = true;

        //�ӶԶ˽�������
        socket_.async_read_some(boost::asio::null_buffers(), 
            boost::bind(&TcpClientSocketEx::HandleRead, this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));

       if (connected_handler_)
       {
           connected_handler_();
       }
    }
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
void TcpClientSocketEx::HandleConnect(const boost::system::error_code& error)
{
    if (!error) //������
    {
        //���������ϱ�ʶ
        connected_ = true;

        //�ӶԶ˽�������
        socket_.async_read_some(boost::asio::null_buffers(), 
            boost::bind(&TcpClientSocketEx::HandleRead, this, 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
	else if (disconnected_handler_)
	{
		disconnected_handler_();
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
bool TcpClientSocketEx::IsConnected() const
{
    return connected_;
}

/*------------------------------------------------------------------------------
 *��  ��: ���ӶԶ��յ����ݺ�Ļص�����
 *��  ��: [in]error �������
          [in]bytes_transferred �յ�������ֽ���
 *����ֵ:
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::HandleRead(const boost::system::error_code& error, 
        uint64 /*bytes_transferred*/)
{
	size_t available = socket_.available(); //���Զ�ȡ���ֽ���
	if (error || (available == 0))
	{
		connected_ = false;
        socket_.close();
		if (disconnected_handler_)
		{
			disconnected_handler_();
		}

		return ;
	}

    auto handler = receive_handler(); //�������ݺ�Ļص�����
    if (handler)
    {
        receive_buffer_.resize(available);
        socket_.read_some(boost::asio::buffer(receive_buffer_)); //ͬ����ȡ����
        handler(error, (available > 0 ? &receive_buffer_[0] : nullptr), available);
    }
 
    //�����ӶԶ˽�������
    socket_.async_read_some(boost::asio::null_buffers(), 
        boost::bind(&TcpClientSocketEx::HandleRead, this, 
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
void TcpClientSocketEx::Close()
{
    socket_.close(); //ͬ���ر��׽���
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
bool TcpClientSocketEx::Send(const char* data, uint64 length)
{
	if (connected_)
	{
		io_service().post(boost::bind(&TcpClientSocketEx::DoSend, this, data, length));
	}
	
    return connected_;
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
void TcpClientSocketEx::DoSend(const char* data, uint64 length)
{
    boost::asio::async_write(socket_, boost::asio::buffer(data, length), 
        boost::bind(&TcpClientSocketEx::HandleSent, this, 
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
void TcpClientSocketEx::HandleSent(const boost::system::error_code& error, uint64 bytes_transferred)
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
EndPoint TcpClientSocketEx::local_address() const
{
    return to_endpoint(socket_.local_endpoint());
}

/*------------------------------------------------------------------------------
 * ��  ��: ���������Ķ�ʱ��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::OnTimer()
{
	if (!connected_)
	{
		Connect(); //���ӶԶ�
	}
}

}
