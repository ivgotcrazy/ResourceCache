/*##############################################################################
 * 文件名   : communicator.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.15
 * 文件描述 : 类Communicator和MsgInfoManager的声明文件
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_COMMUNICATOR
#define HEADER_COMMUNICATOR

#include "ugs_typedef.hpp"
#include "endpoint.hpp"
#include "singleton.hpp"
#include "server_socket.hpp"
#include "msg_dispatcher.hpp"
#include "message_recombiner.hpp"

namespace BroadCache
{

namespace Detail
{

/*******************************************************************************
 *描  述: UGS和RCS通信对象，它们以消息PbMessage的形式通信
 *作  者: rosan
 *时  间: 2013.11.15
 ******************************************************************************/
class Communicator
{
public:
    // 注册（从RCS发送过来的）消息回调函数
    template<class MsgType>
    void RegisterMsgHandler(const boost::function<void(const MsgType&, // 具体的消息实体
                            const EndPoint&,  // 发送此消息的RCS
                            const msg_seq_t&)>& func);  // 此消息对应的序列号

    // 注册（从RCS发送过来的）消息回调函数
    template<class MsgType>
    void RegisterMsgCommonHandler(const boost::function<void(const PbMessage&,  // 抽象消息实体
                                  const EndPoint&,  // 发送此消息的RCS
                                  const msg_seq_t&)>& func);  // 此消息对应的序列号
                        

    // 向RCS发送消息
    void SendMsg(const PbMessage& msg, const EndPoint& endpoint, msg_seq_t seq);

    // 初始化此对象
    bool Init(boost::asio::io_service& ios);

private:
    class MsgInfoManager;

private:
    // 从套接字上接收到数据后的回调函数
    void OnReceive(const EndPoint& ep, 
                   const error_code& error, 
                   const char* data, 
                   uint64 length);

private:
    static const size_t kBufSize = 4096;  // 从RCS接收数据的缓冲区大小

    EndPoint last_msg_sender_;  // 最近一个消息的发送者
    msg_seq_t last_msg_seq_ = 0;  // 最近一个消息的序列号

    char buf_[sizeof(msg_seq_t) + kBufSize];  // 从RCS接收数据的缓冲区（序列号 + 实际数据）
    char* data_ = buf_ + sizeof(msg_seq_t);  // 从RCS接收到的实际数据部分（不包括序列号）

    MsgDispatcher msg_dispatcher_;  // 消息分发器
    boost::shared_ptr<TcpServerSocket> socket_;  // 向RCS发送数据的套接字
    MessageRecombiner message_recombiner_;
    std::list<std::vector<char> > buffers_;

	boost::mutex mutex_;
};

/*------------------------------------------------------------------------------
 * 描  述: 注册（从RCS发送过来的）消息回调函数
 * 参  数: [in] func 消息回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.11.15
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class MsgType>
void Communicator::RegisterMsgHandler(const boost::function<void(const MsgType&, // 具体的消息实体
                    const EndPoint&,  // 发送此消息的RCS
                    const msg_seq_t&)>& func)  // 此消息对应的序列号
{
    msg_dispatcher_.RegisterCallback<MsgType>(
        boost::bind(func, _1, boost::ref(last_msg_sender_), boost::ref(last_msg_seq_)));
}

/*------------------------------------------------------------------------------
 * 描  述: 注册（从RCS发送过来的）消息回调函数
 * 参  数: [in] func 消息回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.11.15
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class MsgType>
void Communicator::RegisterMsgCommonHandler(const boost::function<void(const PbMessage&, // 抽象的消息实体
                    const EndPoint&,  // 发送此消息的RCS
                    const msg_seq_t&)>& func)  // 此消息对应的序列号
{
    msg_dispatcher_.RegisterCommonCallback<MsgType>(
        boost::bind(func, _1, boost::ref(last_msg_sender_), boost::ref(last_msg_seq_)));
}

}  // namespace Detail

typedef Singleton<Detail::Communicator> Communicator;

}  // namespace BroadCache

#endif  // HEADER_COMMUNICATOR
