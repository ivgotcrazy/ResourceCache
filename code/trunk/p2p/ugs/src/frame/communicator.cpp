/*##############################################################################
 * �ļ���   : communicator.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.15
 * �ļ����� : Communicator��MsgInfoManager���ʵ���ļ�
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "communicator.hpp"
#include "protobuf_msg_encode.hpp"
#include "bc_util.hpp"
#include "net_byte_order.hpp"
#include "ugs_config_parser.hpp"

namespace BroadCache
{
namespace Detail
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
 -----------------------------------------------------------------------------*/ 
static msg_length_t GetCommunicatorMsgLength(const char* data, msg_length_t length)
{
    return msg_length_t(NetToHost<int32>(data + sizeof(msg_seq_t))
        + sizeof(int32) + sizeof(msg_seq_t));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��RCS������Ϣ
 * ��  ��: [in] msg Ҫ���͵���Ϣ����
 *         [in] endpoint RCS��ַ
 *         [in] seq ��Ϣ���к�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void Communicator::SendMsg(const PbMessage& msg, const EndPoint& endpoint, msg_seq_t seq)
{
	boost::mutex::scoped_lock lock(mutex_);
	
    while (buffers_.size() > 100)
    {
        buffers_.pop_front();
    }

    buffers_.push_back(std::vector<char>());
    buffers_.back().resize(sizeof(msg_seq_t) + kBufSize);
    char* buf = &buffers_.back()[0];

    memcpy(buf, &seq, sizeof(msg_seq_t));
    
    msg_length_t msg_length = EncodeProtobufMessage(msg, buf + sizeof(msg_seq_t), kBufSize);
    if (msg_length != 0)
    {
        socket_->Send(endpoint, buf, sizeof(msg_seq_t) + msg_length);
    }
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ʼ���˶���
 * ��  ��: [out] ios IO�������
 *         [in] port ���׽��ְ󶨵Ķ˿ں�
 * ����ֵ: ��ʼ���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool Communicator::Init(boost::asio::io_service& ios)
{
    message_recombiner_.SetMessageLengthFunction(&GetCommunicatorMsgLength);

    std::string endpoint_str;
    GET_CONFIG_STR(UgsConfigParser, "global.common.listen-address", endpoint_str);

    EndPoint endpoint;
    bool ok = ParseEndPoint(endpoint_str, endpoint);
    if (ok)
    {
        socket_.reset(new TcpServerSocket(endpoint.port, ios,
            boost::bind(&Communicator::OnReceive, this, _1, _2, _3, _4)));

        LOG(INFO) << "UGS Communicator bind address : " << endpoint;
    }
    else
    {
        LOG(INFO) << "Config global.common.listen-address is invalid.";
    }

    return ok;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��RCS���յ����ݺ�Ļص�����
 * ��  ��: [in] ep RCS��ַ
 *         [in] error �������
 *         [in] data ���յ�������
 *         [in] length ���յ������ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void Communicator::OnReceive(const EndPoint& ep, 
                   const error_code& error, 
                   const char* data, 
                   uint64 length)
{
    if (error || (length == 0) || (data == nullptr))
        return ;

    MessageList msgs = message_recombiner_.GetMessages(data, length);
    FOREACH(e, msgs)
    {
        last_msg_sender_ = ep;
        memcpy(&last_msg_seq_, e.msg_data, sizeof(msg_seq_t));

        auto msg = DecodeProtobufMessage(e.msg_data + sizeof(msg_seq_t), e.msg_length - sizeof(msg_seq_t));
        if (msg)
        {
            msg_dispatcher_.Dispatch(*msg);
        } 
    }
}

}  // namespace Detail
}  // namespace BroadCache
