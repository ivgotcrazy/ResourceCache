/*------------------------------------------------------------------------------
 * 文件名   : communicator.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.08.28
 * 文件描述 : 实现类Communicator
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描  述: 获取communicator之间通信一条消息的长度
 * 参  数: [in] data 消息数据
 *         [in] length 消息数据长度（可能是多条消息的合并后的数据）
 * 返回值: 一条消息长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 *-----------------------------------------------------------------------------*/ 
static msg_length_t GetCommunicatorMsgLength(const char* data, msg_length_t length)
{
        return msg_length_t(NetToHost<int32>(data + sizeof(Communicator::msg_seq_t))
                + sizeof(int32) + sizeof(Communicator::msg_seq_t));
}

/*------------------------------------------------------------------------------
 * 描  述: 获取Communicator单例
 * 参  数: 
 * 返回值: Communicator单例
 * 修  改:
 *   时间 2013年10月25日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
Communicator& Communicator::GetInstance()
{
	static Communicator communicator;
	return communicator;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取单例前需要初始化
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 获取一个随机的消息序列号
 * 参  数: 
 * 返回值: 随机的消息序列号
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
Communicator::msg_seq_t Communicator::RandomSeq()
{ 
    return (rand() << 16) | (rand() & 0xffff);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取一个唯一的消息序列号
 * 参  数: 
 * 返回值: 唯一的消息序列号
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
Communicator::msg_seq_t Communicator::GetUniqueSeq() const
{ 
    msg_seq_t seq = RandomSeq();  // 随机产生一个序列号
    while ((seq == kDefaultMsgSeq)  // 是否是默认序列号
        || (receive_handlers_.find(seq) != receive_handlers_.end()))  // 此序列号是否已经被占用了
    {
        seq = RandomSeq();  // 继续产生一个序列号
    } 

    return seq;
}

/*------------------------------------------------------------------------------
 * 描  述: 向对方发送信息
 * 参  数: [in] msg 消息对象
 *         [in] handler 此消息的回复消息的回调函数
 * 返回值: 
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
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

    // 如果发送一个不需要回复的消息就用默认序列号，否则产生一个唯一的序列
    msg_seq_t seq = (handler ? GetUniqueSeq() : kDefaultMsgSeq);
    if (handler)
    {
        receive_handlers_.insert(std::make_pair(seq, handler));
    }

    memcpy(buf, &seq, sizeof(msg_seq_t));  // 添加序列号
    msg_length_t msg_length = EncodeProtobufMessage(msg, 
        buf + sizeof(msg_seq_t), BUF_SIZE);

    if (msg_length != 0)
    {
        socket_->Send(buf, sizeof(msg_seq_t) + msg_length);
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 从对端收到回复后的回调函数
 * 参  数: [in] error 错误代码
 *         [in] data 收到的数据
 *         [in] length 收到数据的长度
 * 返回值:
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
void Communicator::OnReceive(const error_code& error, const char* data, size_t length)
{
    if (error || (data == nullptr) || (length == 0))  // 是否是接收数据失败
	{
        return ;
    }

    MessageList msgs = message_recombiner_.GetMessages(data, length);
    FOREACH(e, msgs)
    {
        msg_seq_t seq = 0;  // 获取消息序列
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
 * 描  述: 设置UGS连接上后的回调函数 
 * 参  数: [in] handler UGS连接上后的回调函数 
 * 返回值:
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void Communicator::RegisterConnectedHandler(const ConnectedHandler& handler)
{
    connected_handlers_.push_back(handler);
}

/*------------------------------------------------------------------------------
 * 描  述: 设置UGS失去连接后的回调函数 
 * 参  数: [in] handler UGS失去连接后的回调函数 
 * 返回值:
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void Communicator::SetDisconnectedHandler(const DisconnectedHandler& handler)
{
    disconnected_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 当UGS连接上后的回调函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 当UGS失去连接后的回调函数
 * 参  数:
 * 返回值:
 * �  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
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
