/*#############################################################################
 * �ļ���   : tracker_request_manager.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��08��24��
 * �ļ����� : TrackerManager����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * �������������ڹ�������TrackerRequest��ȷ������TrackerRequest����˳����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��10��30��
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

