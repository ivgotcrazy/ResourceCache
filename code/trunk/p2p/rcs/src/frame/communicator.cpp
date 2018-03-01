/*------------------------------------------------------------------------------
 * ÎÄ¼şÃû   : communicator.cpp
 * ´´½¨ÈË   : rosan 
 * ´´½¨Ê±¼ä : 2013.08.28
 * ÎÄ¼şÃèÊö : ÊµÏÖÀàCommunicator
 * °æÈ¨ÉùÃ÷ : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#include "communicator.hpp"
#include <cstdlib>
#include <boost/bind.hpp>
#include <glog/logging.h>
#include "protobuf_msg_encode.hpp"
#include "message.pb.h"
#include "rcs_config_parser.hpp"
#include "bc_assert.hpp"
#include "rcs_util.hpp"
#include "net_byte_order.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡communicatorÖ®¼äÍ¨ĞÅÒ»ÌõÏûÏ¢µÄ³¤¶È
 * ²Î  Êı: [in] data ÏûÏ¢Êı¾İ
 *         [in] length ÏûÏ¢Êı¾İ³¤¶È£¨¿ÉÄÜÊÇ¶àÌõÏûÏ¢µÄºÏ²¢ºóµÄÊı¾İ£©
 * ·µ»ØÖµ: Ò»ÌõÏûÏ¢³¤¶È
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.11.23
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 *-----------------------------------------------------------------------------*/ 
static msg_length_t GetCommunicatorMsgLength(const char* data, msg_length_t length)
{
        return msg_length_t(NetToHost<int32>(data + sizeof(Communicator::msg_seq_t))
                + sizeof(int32) + sizeof(Communicator::msg_seq_t));
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡Communicatorµ¥Àı
 * ²Î  Êı: 
 * ·µ»ØÖµ: Communicatorµ¥Àı
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ25ÈÕ
 *   ×÷Õß teck_zhou
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/
Communicator& Communicator::GetInstance()
{
	static Communicator communicator;
	return communicator;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡µ¥ÀıÇ°ĞèÒª³õÊ¼»¯
 * ²Î  Êı: 
 * ·µ»ØÖµ: 
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.08.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/
bool Communicator::Init(io_service& ios)
{
	std::string str_addr;
    GET_RCS_CONFIG_STR("global.common.ugs", str_addr);

	auto i = str_addr.find(':');
	if ((i == 0) || (i == std::string::npos) || (i == str_addr.size() - 1))
	{
		LOG(ERROR) << "UGS address format error.";
		return false;
	}

	EndPoint endpoint(boost::asio::ip::address::from_string(str_addr.substr(0, i)).to_v4().to_ulong(), 
		boost::lexical_cast<unsigned long>(str_addr.substr(i + 1)));

	socket_.reset(new TcpClientSocketEx(ios, endpoint));
    socket_->set_receive_handler(boost::bind(&Communicator::OnReceive, this, _1, _2, _3));
    socket_->set_connected_handler(boost::bind(&Communicator::OnConnected, this));
    socket_->set_disconnected_handler(boost::bind(&Communicator::OnDisconnected, this));

    message_recombiner_.SetMessageLengthFunction(&GetCommunicatorMsgLength);

	initialized_ = true;

    return true;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡Ò»¸öËæ»úµÄÏûÏ¢ĞòÁĞºÅ
 * ²Î  Êı: 
 * ·µ»ØÖµ: Ëæ»úµÄÏûÏ¢ĞòÁĞºÅ
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.08.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/
Communicator::msg_seq_t Communicator::RandomSeq()
{ 
    return (rand() << 16) | (rand() & 0xffff);
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡Ò»¸öÎ¨Ò»µÄÏûÏ¢ĞòÁĞºÅ
 * ²Î  Êı: 
 * ·µ»ØÖµ: Î¨Ò»µÄÏûÏ¢ĞòÁĞºÅ
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.08.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/
Communicator::msg_seq_t Communicator::GetUniqueSeq() const
{ 
    msg_seq_t seq = RandomSeq();  // Ëæ»ú²úÉúÒ»¸öĞòÁĞºÅ
    while ((seq == kDefaultMsgSeq)  // ÊÇ·ñÊÇÄ¬ÈÏĞòÁĞºÅ
        || (receive_handlers_.find(seq) != receive_handlers_.end()))  // ´ËĞòÁĞºÅÊÇ·ñÒÑ¾­±»Õ¼ÓÃÁË
    {
        seq = RandomSeq();  // ¼ÌĞø²úÉúÒ»¸öĞòÁĞºÅ
    } 

    return seq;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: Ïò¶Ô·½·¢ËÍĞÅÏ¢
 * ²Î  Êı: [in] msg ÏûÏ¢¶ÔÏó
 *         [in] handler ´ËÏûÏ¢µÄ»Ø¸´ÏûÏ¢µÄ»Øµ÷º¯Êı
 * ·µ»ØÖµ: 
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.08.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/
void Communicator::SendMsg(const PbMessage& msg, const ReceiveHandler& handler)
{
	BC_ASSERT(initialized_);

	boost::mutex::scoped_lock lock(mutex_);

    while (buffers_.size() > 50)
    {
        buffers_.pop_front();
    }

    buffers_.push_back(std::vector<char>());
    buffers_.back().resize(sizeof(msg_seq_t) + BUF_SIZE);
    char* buf = &buffers_.back()[0];

    // Èç¹û·¢ËÍÒ»¸ö²»ĞèÒª»Ø¸´µÄÏûÏ¢¾ÍÓÃÄ¬ÈÏĞòÁĞºÅ£¬·ñÔò²úÉúÒ»¸öÎ¨Ò»µÄĞòÁĞ
    msg_seq_t seq = (handler ? GetUniqueSeq() : kDefaultMsgSeq);
    if (handler)
    {
        receive_handlers_.insert(std::make_pair(seq, handler));
    }

    memcpy(buf, &seq, sizeof(msg_seq_t));  // Ìí¼ÓĞòÁĞºÅ
    msg_length_t msg_length = EncodeProtobufMessage(msg, 
        buf + sizeof(msg_seq_t), BUF_SIZE);

    if (msg_length != 0)
    {
        socket_->Send(buf, sizeof(msg_seq_t) + msg_length);
    }
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ´Ó¶Ô¶ËÊÕµ½»Ø¸´ºóµÄ»Øµ÷º¯Êı
 * ²Î  Êı: [in] error ´íÎó´úÂë
 *         [in] data ÊÕµ½µÄÊı¾İ
 *         [in] length ÊÕµ½Êı¾İµÄ³¤¶È
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.08.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/
void Communicator::OnReceive(const error_code& error, const char* data, size_t length)
{
    if (error || (data == nullptr) || (length == 0))  // ÊÇ·ñÊÇ½ÓÊÕÊı¾İÊ§°Ü
	{
        return ;
    }

    MessageList msgs = message_recombiner_.GetMessages(data, length);
    FOREACH(e, msgs)
    {
        msg_seq_t seq = 0;  // »ñÈ¡ÏûÏ¢ĞòÁĞ
        memcpy(&seq, e.msg_data, sizeof(msg_seq_t));  

        auto msg = DecodeProtobufMessage(e.msg_data + sizeof(msg_seq_t), e.msg_length - sizeof(msg_seq_t));
        if (msg)
        {
            auto i = receive_handlers_.find(seq);
            if (i != receive_handlers_.end())
            {
                (i->second)(*msg);
                receive_handlers_.erase(i);
            }
        }
    }
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ÉèÖÃUGSÁ¬½ÓÉÏºóµÄ»Øµ÷º¯Êı 
 * ²Î  Êı: [in] handler UGSÁ¬½ÓÉÏºóµÄ»Øµ÷º¯Êı 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.11.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void Communicator::RegisterConnectedHandler(const ConnectedHandler& handler)
{
    connected_handlers_.push_back(handler);
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: ÉèÖÃUGSÊ§È¥Á¬½ÓºóµÄ»Øµ÷º¯Êı 
 * ²Î  Êı: [in] handler UGSÊ§È¥Á¬½ÓºóµÄ»Øµ÷º¯Êı 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.11.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void Communicator::SetDisconnectedHandler(const DisconnectedHandler& handler)
{
    disconnected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: µ±UGSÁ¬½ÓÉÏºóµÄ»Øµ÷º¯Êı
 * ²Î  Êı:
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013.11.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void Communicator::OnConnected()
{
    LOG(INFO) << "UGS connected.";

    FOREACH(handler, connected_handlers_)
    {
        handler();
    }
}

/*------------------------------------------------------------------------------
 * Ãè  Êö: µ±UGSÊ§È¥Á¬½ÓºóµÄ»Øµ÷º¯Êı
 * ²Î  Êı:
 * ·µ»ØÖµ:
 * Ş  ¸Ä:
 *   Ê±¼ä 2013.11.28
 *   ×÷Õß rosan
 *   ÃèÊö ´´½¨
 -----------------------------------------------------------------------------*/ 
void Communicator::OnDisconnected()
{
    LOG(INFO) << "UGS disconnected.";

    if (disconnected_handler_)
    {
        disconnected_handler_();
    }
}

}
