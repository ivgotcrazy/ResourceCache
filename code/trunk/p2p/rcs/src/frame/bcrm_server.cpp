/*#############################################################################
 * 文件名   : bcrm_server.cpp
 * 创建人   : vicent_pan	
 * 创建时间 : 2013/10/19
 * 文件描述 : 远程监测诊断工具服务端相关类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bcrm_server.hpp"
#include "policy.hpp"
#include "protobuf_msg_encode.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
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
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/	
BcrmServer::~BcrmServer()
{
	bcrm_thread_.join();       
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动函数接口
 * 参  数: [in]ios   io_server
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BcrmServer::Start(boost::asio::io_service & ios)
{	
	GET_RCS_CONFIG_STR("global.common.bcrm-listen-port", listen_port_);

	LOG(INFO)<< "Start bcrm server thread | listen port: " <<listen_port_;
	
	bcrm_thread_ = boost::thread(boost::bind(&BcrmServer::Run, this, boost::ref(ios)));
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动线程的执行函数
 * 参  数: [in]ios   io_servr
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::Run(boost::asio::io_service & ios)
{	
	socket_ = new TcpServerSocket(boost::lexical_cast<unsigned short>(listen_port_), ios, 
		                         boost::bind(&BcrmServer::DoReceive, this, _1, _2, _3, _4));
}


/*-----------------------------------------------------------------------------
 * 描  述: 发送函数
 * 参  数: [in] ep  对端地址
 		   [in] info  传送数据的首地址
 		   [in] length 传送数据的长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::DoSend(const EndPoint & ep, const std::vector<char>& info, uint32 length)
{
	socket_->Send(ep, &info[0], length);
}

/*-----------------------------------------------------------------------------
 * 描  述: 接收函数 tcpserversocket的接收回调函数
 * 参  数: [in] ep 端地址
 		   [in] error 错误信息
 		   [in] data  接收信息首地址
 		   [in] length 接收信息长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
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
 * 描  述: 处理从对端发过来的消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::HandleMsg(const PbMessage& msg)
{
	 msg_dispatcher_.Dispatch(msg);
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到对端传送过的session请求消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestSessionMsg(const RequestSessionMsg & msg)
{
	get_info_->SetSessionInfo();
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到对端传送过的torrent请求消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestTorrentMsg(const RequestTorrentMsg & msg)
{
	get_info_->SetTorrentInfo(msg.session_type());
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到对端传送过的peer请求消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestPeerMsg(const RequestPeerMsg & msg)
{
	get_info_->SetPeerInfo(msg.session_type(), msg.info_hash());
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到对端传送过的system请求消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestSystemMsg(const RequestSystemMsg & msg)
{
	get_info_->SetSystemInfo();
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到对端传送过的cache请求消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BcrmServer::OnRequestCacheStatMsg(const RequestCacheStatMsg & msg)
{
	get_info_->SetCacheStatusInfo(msg.session_type());
}

}
