/*#############################################################################
 * 文件名   : bcrm_server.hpp
 * 创建人   : vicent_pan	
 * 创建时间 : 2013年10月19日
 * 文件描述 : 远程监测诊断工具服务端相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描述: bcrm 服务端 远程监测诊断工具
 * 作者：vincent_pan
 * 时间：2013/11/02
 *****************************************************************************/
class BcrmServer : public boost::noncopyable
{	
public:

	BcrmServer(SessionManager & sm, MonitorManager & srm);

	~BcrmServer();

	bool Start(boost::asio::io_service & ios);	//启动bcrmserver执行线程,读取配置文件

private:
	void Run(boost::asio::io_service & ios);    //启动bcrmserver

	void DoCommandMsg(const EndPoint& ep);      //控制commands，调用相应的接口获得info 在进行send();

	void DoSend(const EndPoint& ep, const std::vector<char>& info, uint32 length);

	void DoReceive(const EndPoint& ep, const error_code& error, const char* data, uint32 length);

	void HandleMsg(const PbMessage& msg);

	void OnRequestSessionMsg(const RequestSessionMsg& msg);
	void OnRequestTorrentMsg(const RequestTorrentMsg& msg);
	void OnRequestPeerMsg(const RequestPeerMsg& msg);
	void OnRequestSystemMsg(const RequestSystemMsg& msg);
	void OnRequestCacheStatMsg(const RequestCacheStatMsg& msg);

private:
	std::string listen_port_;					//监听端口
	TcpServerSocket * socket_;                  //Tcpserver 的指针
	boost::thread bcrm_thread_;                 //bcrmserver 线程
	boost::scoped_ptr<GetBcrmInfo>  get_info_;                              
	MsgDispatcher msg_dispatcher_;
};

}

#endif
