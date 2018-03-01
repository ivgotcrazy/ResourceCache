/*------------------------------------------------------------------------------
 * 文件名   : communicator.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.08.28
 * 文件描述 : 此文件主要包含Communicator类
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#ifndef HEADER_COMMUNICATOR
#define HEADER_COMMUNICATOR

#include <queue>
#include <boost/function.hpp>
#include <boost/unordered_map.hpp>
#include "client_socket_ex.hpp"
#include "message_recombiner.hpp"
#include "timer.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: 此类主要实现RCS和UGS以信息的形式通信
 *作  者: rosan
 *时  间: 2013.08.28
 -----------------------------------------------------------------------------*/
class Communicator
{
public:
    enum { BUF_SIZE = 4096 };

    typedef int msg_seq_t;
    typedef boost::function<void(const PbMessage&)> ReceiveHandler;
    typedef boost::function<void()> ConnectedHandler;
    typedef boost::function<void()> DisconnectedHandler;

	static Communicator& GetInstance();

	bool Init(io_service& ios);
    void SendMsg(const PbMessage& msg, const ReceiveHandler& handler = ReceiveHandler());

    void RegisterConnectedHandler(const ConnectedHandler& handler);
    void SetDisconnectedHandler(const DisconnectedHandler& handler);
    
private:
	Communicator() : initialized_(false) {}
	Communicator(const Communicator&) {}
	bool operator=(const Communicator&) { return true; }

    void OnReceive(const error_code& error, const char* data, size_t length);

    static msg_seq_t RandomSeq();
    msg_seq_t GetUniqueSeq() const;

    void OnConnected();
    void OnDisconnected();

private:
    //默认消息序列号，用于发送那些不需要对方回复的消息
    //其余的消息序列号，用于发送那些需要对方回复的消息
    //发送需要对方回复的消息，请不要使用默认消息序列号
    static const msg_seq_t kDefaultMsgSeq = 0;

    char buf_[sizeof(msg_seq_t) + BUF_SIZE]; //发送消息的缓冲区
    boost::scoped_ptr<TcpClientSocketEx> socket_; //通信tcp socket
    boost::unordered_map<msg_seq_t, ReceiveHandler> receive_handlers_; //接收对端的消息回调函数
    std::list<std::vector<char> > buffers_;
    MessageRecombiner message_recombiner_;
    std::list<ConnectedHandler> connected_handlers_;
    DisconnectedHandler disconnected_handler_;

	boost::mutex mutex_;

	bool initialized_;
};

}

#endif
