/*#############################################################################
 * �ļ���   : bcrm_server.hpp
 * ������   : vicent_pan	
 * ����ʱ�� : 2013��10��19��
 * �ļ����� : Զ�̼����Ϲ��߷�������������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BCRM_SERVER
#define HEADER_BCRM_SERVER

#include "server_socket.hpp"
#include "session_manager.hpp"
#include "depend.hpp"
#include "rcs_config_parser.hpp"
#include "monitor_manager.hpp"
#include "get_bcrm_info.hpp"
#include "msg_dispatcher.hpp"
#include "bcrm_message.pb.h"

namespace BroadCache 
{

class SessionManager;
class MonitorManager;
class GetBcrmInfo;

typedef int msg_seq_t;
/******************************************************************************
 * ����: bcrm ����� Զ�̼����Ϲ���
 * ���ߣ�vincent_pan
 * ʱ�䣺2013/11/02
 *****************************************************************************/
class BcrmServer : public boost::noncopyable
{	
public:

	BcrmServer(SessionManager & sm, MonitorManager & srm);

	~BcrmServer();

	bool Start(boost::asio::io_service & ios);	//����bcrmserverִ���߳�,��ȡ�����ļ�

private:
	void Run(boost::asio::io_service & ios);    //����bcrmserver

	void DoCommandMsg(const EndPoint& ep);      //����commands��������Ӧ�Ľӿڻ��info �ڽ���send();

	void DoSend(const EndPoint& ep, const std::vector<char>& info, uint32 length);

	void DoReceive(const EndPoint& ep, const error_code& error, const char* data, uint32 length);

	void HandleMsg(const PbMessage& msg);

	void OnRequestSessionMsg(const RequestSessionMsg& msg);
	void OnRequestTorrentMsg(const RequestTorrentMsg& msg);
	void OnRequestPeerMsg(const RequestPeerMsg& msg);
	void OnRequestSystemMsg(const RequestSystemMsg& msg);
	void OnRequestCacheStatMsg(const RequestCacheStatMsg& msg);

private:
	std::string listen_port_;					//�����˿�
	TcpServerSocket * socket_;                  //Tcpserver ��ָ��
	boost::thread bcrm_thread_;                 //bcrmserver �߳�
	boost::scoped_ptr<GetBcrmInfo>  get_info_;                              
	MsgDispatcher msg_dispatcher_;
};

}

#endif
