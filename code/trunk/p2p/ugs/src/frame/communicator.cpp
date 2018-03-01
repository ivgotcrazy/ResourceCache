/*##############################################################################
 * 文件名   : communicator.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.15
 * 文件描述 : Communicator和MsgInfoManager类的实现文件
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
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
 * 描  述: 获取communicator之间通信一条消息的长度
 * 参  数: [in] data 消息数据
 *         [in] length 消息数据长度（可能是多条消息的合并后的数据）
 * 返回值: 一条消息长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static msg_length_t GetCommunicatorMsgLength(const char* data, msg_length_t length)
{
    return msg_length_t(NetToHost<int32>(data + sizeof(msg_seq_t))
        + sizeof(int32) + sizeof(msg_seq_t));
}

/*------------------------------------------------------------------------------
 * 描  述: 向RCS发送消息
 * 参  数: [in] msg 要发送的消息对象
 *         [in] endpoint RCS地址
 *         [in] seq 消息序列号
 * 返回值:
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 初始化此对象
 * 参  数: [out] ios IO服务对象
 *         [in] port 此套接字绑定的端口号
 * 返回值: 初始化是否成功
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 从RCS接收到数据后的回调函数
 * 参  数: [in] ep RCS地址
 *         [in] error 错误代码
 *         [in] data 接收到的数据
 *         [in] length 接收到的数据长度
 * 返回值:
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
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
