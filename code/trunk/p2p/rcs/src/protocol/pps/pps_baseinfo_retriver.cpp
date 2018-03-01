/*#############################################################################
 * 文件名   : pps_baseinfo_retriver.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2014年1月22日
 * 文件描述 : PpsBaseinfoRetriver类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_baseinfo_retriver.hpp"
#include "torrent.hpp"
#include "pps_torrent.hpp"
#include "pps_peer_connection.hpp"
#include "bc_util.hpp"
#include "policy.hpp"
#include "rcs_util.hpp"
#include "tracker_request_manager.hpp"
#include "pps_baseinfo_tracker_request.hpp"

namespace BroadCache
{

const uint64 kPpsBaseInfoRequestTimeout = 2;

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年1月22日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PpsBaseinfoRetriver::PpsBaseinfoRetriver(const TorrentSP& torrent) 
	: torrent_(torrent)
	, retrieving_(false)
	, is_dealed(true)
	, retrieved_baseinfo_(false)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年1月22日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsBaseinfoRetriver::Start()
{
	retrieving_ = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Send Request函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年1月22日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsBaseinfoRetriver::TrySendRequest() 
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	if (!is_dealed) return;

	LOG(INFO) << "TrySend baseinfo request ";

	TrackerRequestManager& tr_manager = TrackerRequestManager::GetInstance();

	EndPoint tracker = trackers_queue_.front();
	
	PpsBaseinfoTrackerRequestSP request(new PpsBaseinfoTrackerRequest(torrent->session(), torrent, 
											shared_from_this(), tracker,
											boost::bind(&PpsBaseinfoRetriver::OnTimeoutProc, this, _1)));
	tr_manager.AddRequest(request);   // start retrive baseinfo tracker

	is_dealed = false;
}

/*-----------------------------------------------------------------------------
 * 描  述: 添加tracker地址函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年1月22日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoRetriver::AddTrackerList(std::set<EndPoint>& trackers)
{
	//获得tracker，并插入到队列中
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	for (const EndPoint& tracker : trackers)
	{
		trackers_queue_.push(tracker);	
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 添加baseinfo函数
 * 参  数: [in] msg  baseinfo消息
 * 返回值:
 * 修  改:
 *   时间 2014年1月22日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoRetriver::AddBaseInfo(PpsBaseInfoMsg msg)
{
	//获得tracker，并插入到队列中
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, torrent);
	pps_torrent->AddBaseInfo(msg);  //tracker-list	

	retrieved_baseinfo_ = true;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 申请TrackerList列表
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年1月22日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoRetriver::ApplyTrackerList()
{
	//获得tracker，并插入到队列中
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, torrent);
	pps_torrent->RetriveTrackerList();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器消息处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsBaseinfoRetriver::TickProc()
{
	if (!retrieving_ && retrieved_baseinfo_) return;

	LOG(INFO) << "PpsBaseinfoRetriver Tick";

	// 如果还没有连接可用，则先获取连接
	if (trackers_queue_.empty())
	{
		ApplyTrackerList();  //申请Tracker之后，直接退出
		return;
	}

	TrySendRequest();
}

/*-----------------------------------------------------------------------------
 * 描  述: 超时处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsBaseinfoRetriver::OnTimeoutProc(EndPoint ep)
{
	trackers_queue_.pop();
	is_dealed = true;	
}

}






