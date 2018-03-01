/*#############################################################################
 * 文件名   : pps_baseinfo_retriver.hpp
 * 创建人   : tom_liu
 * 创建时间 : 2014年1月3日
 * 文件描述 : baseinfo retriver类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEADER_PPS_BASEINFO_RETRIVER
#define HEADER_PPS_BASEINFO_RETRIVER

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "endpoint.hpp"

namespace BroadCache
{

struct PpsBaseInfoMsg;

/******************************************************************************
 * 描述: PpsBaseinfoRetriver描述类，包括baseinfo的相关操作
 * 作者：tom_liu
 * 时间：2014/1/3
 *****************************************************************************/
class PpsBaseinfoRetriver : public boost::noncopyable, 
					public boost::enable_shared_from_this<PpsBaseinfoRetriver>
{
public:
	PpsBaseinfoRetriver(const TorrentSP& torrent);
	
	void Start();
	void TickProc();
	
	bool AddBaseInfo(PpsBaseInfoMsg msg);
	bool AddTrackerList(std::set<EndPoint>& trackers);
	bool ApplyTrackerList();

	void OnTimeoutProc(EndPoint ep);

	bool retrived_baseinfo() { return retrieved_baseinfo_; }

private:
	void TrySendRequest();	
	void TimeoutProc();

private:
	TorrentWP torrent_;

	std::queue<EndPoint> trackers_queue_;

	bool retrieving_;

	bool is_dealed; //当前处理的 tracker_request有没有处理
	
	bool retrieved_baseinfo_;
};

}

#endif

