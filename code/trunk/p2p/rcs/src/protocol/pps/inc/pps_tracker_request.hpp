/*#############################################################################
 * �ļ���   : pps_tracker_request.hpp
 * ������   : tom_liu
 * ����ʱ�� : 2014��1��2��
 * �ļ����� : PpsTrackerRequest����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef PPS_TRACKER_REQUEST_HEAD
#define PPS_TRACKER_REQUEST_HEAD

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "tracker_request.hpp"

#define TIMEOUT  15     //��ʱʱ��

namespace BroadCache
{

struct PpsPeerListMsg
{
	uint32 peer_num;
	std::vector<EndPoint> peer_list;
};

/******************************************************************************
 * ����: pps traker ������  PpsTrackerRequest
 * ���ߣ�tom_liu
 * ʱ�䣺2014/1/2
 *****************************************************************************/
class PpsTrackerRequest: public TrackerRequest
{
public:
	PpsTrackerRequest(Session & session, const TorrentSP & torrent, 
								const EndPoint & traker_address);

private:
	virtual SocketConnectionSP CreateConnection() override;
	virtual bool ConstructSendRequest(SendBuffer& send_buf) override;
	virtual bool ProcessResponse(const char* buf, uint64 len) override;
	virtual void TickProc() override;
	virtual bool IsTimeout() override;

private:
	bool SendPeerListRequest(SendBuffer& send_buf);
	bool SendBaseInfoRequest(SendBuffer& send_buf);
	
	void ParsePeerListResp(const char* buf, uint64 len, PpsPeerListMsg& msg);
	void ProcPeerListResp(const char* buf, uint64 len);

private:
	EndPoint local_address_;
	TorrentWP torrent_;
	Session & session_;
	
	uint32 alive_time_;
	uint32 send_time_;

	uint32 seq_id_;
};

}
#endif 


