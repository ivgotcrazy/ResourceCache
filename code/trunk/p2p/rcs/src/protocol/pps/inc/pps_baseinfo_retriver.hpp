/*#############################################################################
 * �ļ���   : pps_baseinfo_retriver.hpp
 * ������   : tom_liu
 * ����ʱ�� : 2014��1��3��
 * �ļ����� : baseinfo retriver������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ����: PpsBaseinfoRetriver�����࣬����baseinfo����ز���
 * ���ߣ�tom_liu
 * ʱ�䣺2014/1/3
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

	bool is_dealed; //��ǰ����� tracker_request��û�д���
	
	bool retrieved_baseinfo_;
};

}

#endif

