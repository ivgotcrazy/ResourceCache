/*#############################################################################
 * �ļ���   : pps_baseinfo_retriver.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2014��1��22��
 * �ļ����� : PpsBaseinfoRetriver��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��22��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsBaseinfoRetriver::PpsBaseinfoRetriver(const TorrentSP& torrent) 
	: torrent_(torrent)
	, retrieving_(false)
	, is_dealed(true)
	, retrieved_baseinfo_(false)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��22��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsBaseinfoRetriver::Start()
{
	retrieving_ = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Send Request����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��22��
 *   ���� tom_liu
 *   ���� ����
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
 * ��  ��: ���tracker��ַ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��22��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoRetriver::AddTrackerList(std::set<EndPoint>& trackers)
{
	//���tracker�������뵽������
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	for (const EndPoint& tracker : trackers)
	{
		trackers_queue_.push(tracker);	
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���baseinfo����
 * ��  ��: [in] msg  baseinfo��Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��22��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoRetriver::AddBaseInfo(PpsBaseInfoMsg msg)
{
	//���tracker�������뵽������
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, torrent);
	pps_torrent->AddBaseInfo(msg);  //tracker-list	

	retrieved_baseinfo_ = true;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����TrackerList�б�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��22��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoRetriver::ApplyTrackerList()
{
	//���tracker�������뵽������
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, torrent);
	pps_torrent->RetriveTrackerList();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����Ϣ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsBaseinfoRetriver::TickProc()
{
	if (!retrieving_ && retrieved_baseinfo_) return;

	LOG(INFO) << "PpsBaseinfoRetriver Tick";

	// �����û�����ӿ��ã����Ȼ�ȡ����
	if (trackers_queue_.empty())
	{
		ApplyTrackerList();  //����Tracker֮��ֱ���˳�
		return;
	}

	TrySendRequest();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsBaseinfoRetriver::OnTimeoutProc(EndPoint ep)
{
	trackers_queue_.pop();
	is_dealed = true;	
}

}






