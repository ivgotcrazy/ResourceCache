/*------------------------------------------------------------------------------
 * �ļ���   : communicator.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.08.28
 * �ļ����� : ʵ����Communicator
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ��ȡcommunicator֮��ͨ��һ����Ϣ�ĳ���
 * ��  ��: [in] data ��Ϣ����
 *         [in] length ��Ϣ���ݳ��ȣ������Ƕ�����Ϣ�ĺϲ�������ݣ�
 * ����ֵ: һ����Ϣ����
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 *-----------------------------------------------------------------------------*/ 
static msg_length_t GetCommunicatorMsgLength(const char* data, msg_length_t length)
{
        return msg_length_t(NetToHost<int32>(data + sizeof(Communicator::msg_seq_t))
                + sizeof(int32) + sizeof(Communicator::msg_seq_t));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡCommunicator����
 * ��  ��: 
 * ����ֵ: Communicator����
 * ��  ��:
 *   ʱ�� 2013��10��25��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
Communicator& Communicator::GetInstance()
{
	static Communicator communicator;
	return communicator;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ����ǰ��Ҫ��ʼ��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.08.28
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ��ȡһ���������Ϣ���к�
 * ��  ��: 
 * ����ֵ: �������Ϣ���к�
 * ��  ��:
 *   ʱ�� 2013.08.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
Communicator::msg_seq_t Communicator::RandomSeq()
{ 
    return (rand() << 16) | (rand() & 0xffff);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡһ��Ψһ����Ϣ���к�
 * ��  ��: 
 * ����ֵ: Ψһ����Ϣ���к�
 * ��  ��:
 *   ʱ�� 2013.08.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
Communicator::msg_seq_t Communicator::GetUniqueSeq() const
{ 
    msg_seq_t seq = RandomSeq();  // �������һ�����к�
    while ((seq == kDefaultMsgSeq)  // �Ƿ���Ĭ�����к�
        || (receive_handlers_.find(seq) != receive_handlers_.end()))  // �����к��Ƿ��Ѿ���ռ����
    {
        seq = RandomSeq();  // ��������һ�����к�
    } 

    return seq;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��Է�������Ϣ
 * ��  ��: [in] msg ��Ϣ����
 *         [in] handler ����Ϣ�Ļظ���Ϣ�Ļص�����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.08.28
 *   ���� rosan
 *   ���� ����
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

    // �������һ������Ҫ�ظ�����Ϣ����Ĭ�����кţ��������һ��Ψһ������
    msg_seq_t seq = (handler ? GetUniqueSeq() : kDefaultMsgSeq);
    if (handler)
    {
        receive_handlers_.insert(std::make_pair(seq, handler));
    }

    memcpy(buf, &seq, sizeof(msg_seq_t));  // ������к�
    msg_length_t msg_length = EncodeProtobufMessage(msg, 
        buf + sizeof(msg_seq_t), BUF_SIZE);

    if (msg_length != 0)
    {
        socket_->Send(buf, sizeof(msg_seq_t) + msg_length);
    }
}

/*------------------------------------------------------------------------------
 * ��  ��: �ӶԶ��յ��ظ���Ļص�����
 * ��  ��: [in] error �������
 *         [in] data �յ�������
 *         [in] length �յ����ݵĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.08.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
void Communicator::OnReceive(const error_code& error, const char* data, size_t length)
{
    if (error || (data == nullptr) || (length == 0))  // �Ƿ��ǽ�������ʧ��
	{
        return ;
    }

    MessageList msgs = message_recombiner_.GetMessages(data, length);
    FOREACH(e, msgs)
    {
        msg_seq_t seq = 0;  // ��ȡ��Ϣ����
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
 * ��  ��: ����UGS�����Ϻ�Ļص����� 
 * ��  ��: [in] handler UGS�����Ϻ�Ļص����� 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void Communicator::RegisterConnectedHandler(const ConnectedHandler& handler)
{
    connected_handlers_.push_back(handler);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����UGSʧȥ���Ӻ�Ļص����� 
 * ��  ��: [in] handler UGSʧȥ���Ӻ�Ļص����� 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void Communicator::SetDisconnectedHandler(const DisconnectedHandler& handler)
{
    disconnected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��UGS�����Ϻ�Ļص�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.28
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ��UGSʧȥ���Ӻ�Ļص�����
 * ��  ��:
 * ����ֵ:
 * �  ��:
 *   ʱ�� 2013.11.28
 *   ���� rosan
 *   ���� ����
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
