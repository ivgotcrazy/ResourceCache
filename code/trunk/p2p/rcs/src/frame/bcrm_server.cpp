/*#############################################################################
 * �ļ���   : bcrm_server.cpp
 * ������   : vicent_pan	
 * ����ʱ�� : 2013/10/19
 * �ļ����� : Զ�̼����Ϲ��߷���������ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bcrm_server.hpp"
#include "policy.hpp"
#include "protobuf_msg_encode.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
BcrmServer::BcrmServer(SessionManager & sm, MonitorManager& srm):listen_port_(),
                       socket_(NULL), bcrm_thread_(), 
                       get_info_(new GetBcrmInfo(sm,srm))
{
	    msg_dispatcher_.RegisterCallback<RequestSessionMsg>(
        boost::bind(&BcrmServer::OnRequestSessionMsg, this, _1));
        msg_dispatcher_.RegisterCallback<RequestTorrentMsg>(
        boost::bind(&BcrmServer::OnRequestTorrentMsg, this, _1));
		msg_dispatcher_.RegisterCallback<RequestSystemMsg>(
        boost::bind(&BcrmServer::OnRequestSystemMsg, this, _1));
		msg_dispatcher_.RegisterCallback<RequestPeerMsg>(
        boost::bind(&BcrmServer::OnRequestPeerMsg, this, _1));
		msg_dispatcher_.RegisterCallback<RequestCacheStatMsg>(
        boost::bind(&BcrmServer::OnRequestCacheStatMsg, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/	
BcrmServer::~BcrmServer()
{
	bcrm_thread_.join();       
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���������ӿ�
 * ��  ��: [in]ios   io_server
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BcrmServer::Start(boost::asio::io_service & ios)
{	
	GET_RCS_CONFIG_STR("global.common.bcrm-listen-port", listen_port_);

	LOG(INFO)<< "Start bcrm server thread | listen port: " <<listen_port_;
	
	bcrm_thread_ = boost::thread(boost::bind(&BcrmServer::Run, this, boost::ref(ios)));
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����̵߳�ִ�к���
 * ��  ��: [in]ios   io_servr
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::Run(boost::asio::io_service & ios)
{	
	socket_ = new TcpServerSocket(boost::lexical_cast<unsigned short>(listen_port_), ios, 
		                         boost::bind(&BcrmServer::DoReceive, this, _1, _2, _3, _4));
}


/*-----------------------------------------------------------------------------
 * ��  ��: ���ͺ���
 * ��  ��: [in] ep  �Զ˵�ַ
 		   [in] info  �������ݵ��׵�ַ
 		   [in] length �������ݵĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::DoSend(const EndPoint & ep, const std::vector<char>& info, uint32 length)
{
	socket_->Send(ep, &info[0], length);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���պ��� tcpserversocket�Ľ��ջص�����
 * ��  ��: [in] ep �˵�ַ
 		   [in] error ������Ϣ
 		   [in] data  ������Ϣ�׵�ַ
 		   [in] length ������Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::DoReceive(const EndPoint& ep, const error_code& error, const char* data, uint32 length)
{	
	if(error || length == 0 || data == NULL)
    {
      //LOG(INFO) << "Failed to receive data from bcrm client ";
        return ;
    }
    auto msg = DecodeProtobufMessage(data, length);

	HandleMsg(*msg);
	DoSend(ep, get_info_->info_, get_info_->info_length_);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ӶԶ˷���������Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::HandleMsg(const PbMessage& msg)
{
	 msg_dispatcher_.Dispatch(msg);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ��Զ˴��͹���session������Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestSessionMsg(const RequestSessionMsg & msg)
{
	get_info_->SetSessionInfo();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ��Զ˴��͹���torrent������Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestTorrentMsg(const RequestTorrentMsg & msg)
{
	get_info_->SetTorrentInfo(msg.session_type());
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ��Զ˴��͹���peer������Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestPeerMsg(const RequestPeerMsg & msg)
{
	get_info_->SetPeerInfo(msg.session_type(), msg.info_hash());
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ��Զ˴��͹���system������Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestSystemMsg(const RequestSystemMsg & msg)
{
	get_info_->SetSystemInfo();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ��Զ˴��͹���cache������Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestCacheStatMsg(const RequestCacheStatMsg & msg)
{
	get_info_->SetCacheStatusInfo(msg.session_type());
}

}
