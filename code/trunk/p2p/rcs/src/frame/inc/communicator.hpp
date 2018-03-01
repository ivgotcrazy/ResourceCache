/*------------------------------------------------------------------------------
 * �ļ���   : communicator.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.08.28
 * �ļ����� : ���ļ���Ҫ����Communicator��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 *��  ��: ������Ҫʵ��RCS��UGS����Ϣ����ʽͨ��
 *��  ��: rosan
 *ʱ  ��: 2013.08.28
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
    //Ĭ����Ϣ���кţ����ڷ�����Щ����Ҫ�Է��ظ�����Ϣ
    //�������Ϣ���кţ����ڷ�����Щ��Ҫ�Է��ظ�����Ϣ
    //������Ҫ�Է��ظ�����Ϣ���벻Ҫʹ��Ĭ����Ϣ���к�
    static const msg_seq_t kDefaultMsgSeq = 0;

    char buf_[sizeof(msg_seq_t) + BUF_SIZE]; //������Ϣ�Ļ�����
    boost::scoped_ptr<TcpClientSocketEx> socket_; //ͨ��tcp socket
    boost::unordered_map<msg_seq_t, ReceiveHandler> receive_handlers_; //���նԶ˵���Ϣ�ص�����
    std::list<std::vector<char> > buffers_;
    MessageRecombiner message_recombiner_;
    std::list<ConnectedHandler> connected_handlers_;
    DisconnectedHandler disconnected_handler_;

	boost::mutex mutex_;

	bool initialized_;
};

}

#endif
