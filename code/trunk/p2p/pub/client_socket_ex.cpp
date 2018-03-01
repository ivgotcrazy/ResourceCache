/*##############################################################################
 * ÎÄ¼şÃû   : client_socket_ex.cpp
 * ´´½¨ÈË   : rosan 
 * ´´½¨Ê±¼ä : 2013.11.28
 * ÎÄ¼şÃèÊö : TcpClientSocketExµÄÊµÏÖÎÄ¼ş 
 * °æÈ¨ÉùÃ÷ : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "client_socket_ex.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *Ãè  Êö: TcpClientSocketEx¹¹Ôìº¯Êı
 *²Î  Êı: [in]ios IO·şÎñ
        : [in]remote_address ¶Ô¶ËµØÖ·
 *·µ»ØÖµ:
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.22
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
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
 *Ãè  Êö: TcpClientSocketExµÄÎö¹¹º¯Êı
 *²Î  Êı:
 *·µ»ØÖµ:
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.20 
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::~TcpClientSocketEx()
{
	Close();
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡socket¶Ô¶ËµÄµØÖ·
 * ²Î  Êı:
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
EndPoint TcpClientSocketEx::remote_address() const
{
    return remote_address_;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡»Øµ÷º¯Êı
 *         ´Ë»Øµ÷º¯ÊıÊÇ½ÓÊÕ¶Ô¶ËÊı¾İºóµÄ´¦Àíº¯Êı
 * ²Î  Êı:
 * ·µ»ØÖµ: ½ÓÊÕ¶Ô¶ËÊı¾İºóµÄ»Øµ÷º¯Êı
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::ReceiveHandler TcpClientSocketEx::receive_handler() const
{
    return receive_handler_;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ÉèÖÃ»Øµ÷º¯Êı
 *         ´Ë»Øµ÷º¯ÊıÊÇ½ÓÊÕ¶Ô¶ËÊı¾İºóµÄ´¦Àíº¯Êı
 * ²Î  Êı: [in]handler ½ÓÊÕ¶Ô¶ËÊı¾İºóµÄ»Øµ÷º¯Êı
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_receive_handler(const ReceiveHandler& handler)
{
    receive_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡io·şÎñ¶ÔÏó
 * ²Î  Êı:
 * ·µ»ØÖµ: io·şÎñ¶ÔÏó
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
boost::asio::io_service& TcpClientSocketEx::io_service() const
{
    return io_service_;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡»Øµ÷º¯Êı 
 *         ±¾¶ËÊı¾İ·¢ËÍ³öÈ¥ºóµÄ»Øµ÷º¯Êı
 * ²Î  Êı:
 * ·µ»ØÖµ: ±¾¶ËÊı¾İ·¢ËÍ³öÈ¥ºóµÄ»Øµ÷º¯Êı
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::SentHandler TcpClientSocketEx::sent_handler() const
{
    return sent_handler_;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ÉèÖÃ»Øµ÷º¯Êı
 *         ±¾¶ËÊı¾İ·¢ËÍ³öÈ¥ºóµÄ»Øµ÷º¯Êı
 * ²Î  Êı: [in]handler ±¾¶ËÊı¾İ·¢ËÍ³öÈ¥ºóµÄ»Øµ÷º¯Êı
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_sent_handler(const SentHandler& handler)
{
    sent_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡»Øµ÷º¯Êı 
 *         ¶Ô¶ËÁ¬½ÓÉÏµÄ»Øµ÷º¯Êı
 * ²Î  Êı:
 * ·µ»ØÖµ: ¶Ô¶ËÁ¬½ÓÉÏµÄ»Øµ÷º¯Êı
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::ConnectedHandler TcpClientSocketEx::connected_handler() const
{
	return connected_handler_;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ÉèÖÃ»Øµ÷º¯Êı
 *         ¶Ô¶ËÁ¬½ÓÉÏµÄ»Øµ÷º¯Êı
 * ²Î  Êı: [in]handler ¶Ô¶ËÁ¬½ÓÉÏµÄ»Øµ÷º¯Êı
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_connected_handler(const ConnectedHandler& handler)
{
	connected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡»Øµ÷º¯Êı 
 *         µ±¶Ô¶ËÊ§È¥Á¬½ÓºóµÄ»Øµ÷º¯Êı
 * ²Î  Êı:
 * ·µ»ØÖµ: µ±¶Ô¶ËÊ§È¥Á¬½ÓºóµÄ»Øµ÷º¯Êı
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
TcpClientSocketEx::DisconnectedHandler TcpClientSocketEx::disconnected_handler() const
{
	return disconnected_handler_;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ÉèÖÃ»Øµ÷º¯Êı
 *         µ±¶Ô¶ËÊ§È¥Á¬½ÓºóµÄ»Øµ÷º¯Êı
 * ²Î  Êı: [in]handler µ±¶Ô¶ËÊ§È¥Á¬½ÓºóµÄ»Øµ÷º¯Êı
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::set_disconnected_handler(const DisconnectedHandler& handler)
{
	disconnected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 *Ãè  Êö: Á¬½Ó¶Ô¶ËµØÖ·
 *²Î  Êı:
 *·µ»ØÖµ:
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.23 
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
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

        //´Ó¶Ô¶Ë½ÓÊÕÊı¾İ
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
 *Ãè  Êö: µ±·şÎñÆ÷Á¬½ÓÉÏºóµÄ»Øµ÷º¯Êı
 *²Î  Êı: [in]error ´íÎó´úÂë
 *·µ»ØÖµ:
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.22 
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::HandleConnect(const boost::system::error_code& error)
{
    if (!error) //Á¬½ÓÉÏ
    {
        //ÉèÖÃÁ¬½ÓÉÏ±êÊ¶
        connected_ = true;

        //´Ó¶Ô¶Ë½ÓÊÕÊı¾İ
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
 *Ãè  Êö: ÅĞ¶ÏÊÇ·ñÒÑ¾­Á¬ÉÏ¶Ô¶ËµØÖ·
 *²Î  Êı:
 *·µ»ØÖµ: ÊÇ·ñÒÑ¾­Á¬ÉÏ¶Ô¶ËµØÖ·
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.22
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
bool TcpClientSocketEx::IsConnected() const
{
    return connected_;
}

/*------------------------------------------------------------------------------
 *Ãè  Êö: µ±´Ó¶Ô¶ËÊÕµ½Êı¾İºóµÄ»Øµ÷º¯Êı
 *²Î  Êı: [in]error ´íÎó´úÂë
          [in]bytes_transferred ÊÕµ½µÄÊıİ×Ö½ÚÊı
 *·µ»ØÖµ:
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.22
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::HandleRead(const boost::system::error_code& error, 
        uint64 /*bytes_transferred*/)
{
	size_t available = socket_.available(); //¿ÉÒÔ¶ÁÈ¡µÄ×Ö½ÚÊı
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

    auto handler = receive_handler(); //½ÓÊÕÊı¾İºóµÄ»Øµ÷º¯Êı
    if (handler)
    {
        receive_buffer_.resize(available);
        socket_.read_some(boost::asio::buffer(receive_buffer_)); //Í¬²½¶ÁÈ¡Êı¾İ
        handler(error, (available > 0 ? &receive_buffer_[0] : nullptr), available);
    }
 
    //¼ÌĞø´Ó¶Ô¶Ë½ÓÊÕÊı¾İ
    socket_.async_read_some(boost::asio::null_buffers(), 
        boost::bind(&TcpClientSocketEx::HandleRead, this, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ¹Ø±Õ´ËÌ×½Ó×Ö
 * ²Î  Êı:
 * ·µ»ØÖµ: ¹Ø±ÕÌ×½Ó×ÖÊÇ·ñ³É¹¦
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::Close()
{
    socket_.close(); //Í¬²½¹Ø±ÕÌ×½Ó×Ö
}

/*------------------------------------------------------------------------------
 *Ãè  Êö: Ïò¶Ô¶Ë·¢ËÍÊı¾İ
 *²Î  Êı: [in]data Òª·¢ËÍµÄÊı¾İµÄÖ¸Õë
          [in]length Òª·¢ËÍÊı¾İµÄ×Ö½ÚÊı
 *·µ»ØÖµ: ·¢ËÍÊÇ·ñ³É¹¦ 
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.22
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
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
 *Ãè  Êö: Ïò¶Ô¶Ë·¢ËÍÊı¾İ
 *²Î  Êı: [in]data Òª·¢ËÍµÄÊı¾İµÄÖ¸Õë
          [in]length Òª·¢ËÍÊı¾İµÄ×Ö½ÚÊı
 *·µ»ØÖµ: 
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.22
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::DoSend(const char* data, uint64 length)
{
    boost::asio::async_write(socket_, boost::asio::buffer(data, length), 
        boost::bind(&TcpClientSocketEx::HandleSent, this, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/*------------------------------------------------------------------------------
 *Ãè  Êö: µ±Êı¾İÒÑ¾­·¢ËÍ³É¹¦ºóµÄ»Øµ÷º¯Êı
 *²Î  Êı: [in]error ´íÎó´úÂë
 *·µ»ØÖµ: [in]bytes_transferred Êı¾İ³É¹¦·¢ËÍµÄ×Ö½ÚÊı
 *ĞŞ  ¸Ä:
 *  Ê±¼ä 2013.08.22
 *  ×÷Õß rosan
 *  ÃèÊö ´´½¨
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
 * Ãè  Êö: »ñÈ¡Ì×½Ó×Ö±¾¶ËµØÖ·
 * ²Î  Êı:
 * ·µ»ØÖµ: Ì×½Ó×Ö±¾¶ËµØÖ·
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
EndPoint TcpClientSocketEx::local_address() const
{
    return to_endpoint(socket_.local_endpoint());
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ¶ÏÏßÖØÁ¬µÄ¶¨Ê±Æ÷
 * ²Î  Êı:
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.10.14
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void TcpClientSocketEx::OnTimer()
{
	if (!connected_)
	{
		Connect(); //Á¬½Ó¶Ô¶Ë
	}
}

}
