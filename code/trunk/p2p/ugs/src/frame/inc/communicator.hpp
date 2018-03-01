/*##############################################################################
 * �ļ���   : communicator.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.15
 * �ļ����� : ��Communicator��MsgInfoManager�������ļ�
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
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
 *��  ��: UGS��RCSͨ�Ŷ�����������ϢPbMessage����ʽͨ��
 *��  ��: rosan
 *ʱ  ��: 2013.11.15
 ******************************************************************************/
class Communicator
{
public:
    // ע�ᣨ��RCS���͹����ģ���Ϣ�ص�����
    template<class MsgType>
    void RegisterMsgHandler(const boost::function<void(const MsgType&, // �������Ϣʵ��
                            const EndPoint&,  // ���ʹ���Ϣ��RCS
                            const msg_seq_t&)>& func);  // ����Ϣ��Ӧ�����к�

    // ע�ᣨ��RCS���͹����ģ���Ϣ�ص�����
    template<class MsgType>
    void RegisterMsgCommonHandler(const boost::function<void(const PbMessage&,  // ������Ϣʵ��
                                  const EndPoint&,  // ���ʹ���Ϣ��RCS
                                  const msg_seq_t&)>& func);  // ����Ϣ��Ӧ�����к�
                        

    // ��RCS������Ϣ
    void SendMsg(const PbMessage& msg, const EndPoint& endpoint, msg_seq_t seq);

    // ��ʼ���˶���
    bool Init(boost::asio::io_service& ios);

private:
    class MsgInfoManager;

private:
    // ���׽����Ͻ��յ����ݺ�Ļص�����
    void OnReceive(const EndPoint& ep, 
                   const error_code& error, 
                   const char* data, 
                   uint64 length);

private:
    static const size_t kBufSize = 4096;  // ��RCS�������ݵĻ�������С

    EndPoint last_msg_sender_;  // ���һ����Ϣ�ķ�����
    msg_seq_t last_msg_seq_ = 0;  // ���һ����Ϣ�����к�

    char buf_[sizeof(msg_seq_t) + kBufSize];  // ��RCS�������ݵĻ����������к� + ʵ�����ݣ�
    char* data_ = buf_ + sizeof(msg_seq_t);  // ��RCS���յ���ʵ�����ݲ��֣����������кţ�

    MsgDispatcher msg_dispatcher_;  // ��Ϣ�ַ���
    boost::shared_ptr<TcpServerSocket> socket_;  // ��RCS�������ݵ��׽���
    MessageRecombiner message_recombiner_;
    std::list<std::vector<char> > buffers_;

	boost::mutex mutex_;
};

/*------------------------------------------------------------------------------
 * ��  ��: ע�ᣨ��RCS���͹����ģ���Ϣ�ص�����
 * ��  ��: [in] func ��Ϣ�ص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.15
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<class MsgType>
void Communicator::RegisterMsgHandler(const boost::function<void(const MsgType&, // �������Ϣʵ��
                    const EndPoint&,  // ���ʹ���Ϣ��RCS
                    const msg_seq_t&)>& func)  // ����Ϣ��Ӧ�����к�
{
    msg_dispatcher_.RegisterCallback<MsgType>(
        boost::bind(func, _1, boost::ref(last_msg_sender_), boost::ref(last_msg_seq_)));
}

/*------------------------------------------------------------------------------
 * ��  ��: ע�ᣨ��RCS���͹����ģ���Ϣ�ص�����
 * ��  ��: [in] func ��Ϣ�ص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.15
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<class MsgType>
void Communicator::RegisterMsgCommonHandler(const boost::function<void(const PbMessage&, // �������Ϣʵ��
                    const EndPoint&,  // ���ʹ���Ϣ��RCS
                    const msg_seq_t&)>& func)  // ����Ϣ��Ӧ�����к�
{
    msg_dispatcher_.RegisterCommonCallback<MsgType>(
        boost::bind(func, _1, boost::ref(last_msg_sender_), boost::ref(last_msg_seq_)));
}

}  // namespace Detail

typedef Singleton<Detail::Communicator> Communicator;

}  // namespace BroadCache

#endif  // HEADER_COMMUNICATOR
