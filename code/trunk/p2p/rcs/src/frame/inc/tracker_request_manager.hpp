/*#############################################################################
 * 文件名   : tracker_request_manager.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年08月24日
 * 文件描述 : TrackerManager声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_TRACKER_MANAGER
#define HEADER_TRACKER_MANAGER

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "endpoint.hpp"
#include "timer.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述：此类用于管理所有TrackerRequest，确保所有TrackerRequest按照顺序处理
 * 作者：teck_zhou
 * 时间：2013年10月30日
 *****************************************************************************/
class TrackerRequestManager : public boost::noncopyable
{
public:
	static TrackerRequestManager& GetInstance();

	~TrackerRequestManager();

	void Init(io_service& ios);
	void AddRequest(const TrackerRequestSP& request);
	void OnRequestProcessed(const TrackerRequestSP& request);

private:
	TrackerRequestManager();
	TrackerRequestManager(const TrackerRequestManager&) {}
	bool operator=(const TrackerRequestManager&) { return true; }

	void ThreadFunc();
	void OnTick();

private:
	typedef std::queue<TrackerRequestSP> RequestQueue;

	typedef boost::unordered_map<EndPoint, TrackerRequestSP> RequestMap;
	typedef RequestMap::iterator RequestMapIter;

private:
	boost::condition_variable request_condition_;

	RequestQueue request_queue_;
	boost::mutex request_mutex_;
	
	RequestMap processing_requests_;
	boost::recursive_mutex processing_request_mutex_;

	boost::scoped_ptr<boost::thread> thread_;
	bool stop_flag_;
	
	boost::scoped_ptr<Timer> request_timer_;
};


}

#endif

