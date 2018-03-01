/*#############################################################################
 * �ļ���   : pps_baseinfo_tracker_request.hpp
 * ������   : tom_liu
 * ����ʱ�� : 2014��1��2��
 * �ļ����� : PpsBaseInfoTrackerRequest����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef PPS_BASEINFO_TRACKER_REQUEST_HEAD
#define PPS_BASEINFO_TRACKER_REQUEST_HEAD

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "tracker_request.hpp"

#define TIMEOUT  15     //��ʱʱ��

namespace BroadCache
{
/******************************************************************************
 * ����: pps base_info��Ϣ
 * ���ߣ�tom_liu
 * ʱ�䣺2014/1/2
 *****************************************************************************/
struct PpsBaseInfoMsg
{
	uint32 bitfield_size;
	uint32 piece_count;
	uint32 metadata_size;
	uint32 file_size;   //piece_size ��Ŀǰ����Ҫͨ�� file_size �� bitfield_size�������
};

/******************************************************************************
 * ����: pps traker ������  PpsBaseInfoTrackerRequest
 * ���ߣ�tom_liu
 * ʱ�䣺2014/1/2
 *****************************************************************************/
class PpsBaseinfoTrackerRequest: public TrackerRequest
{
public:
	typedef boost::function<void(EndPoint)> TimeoutHandler;
	
	PpsBaseinfoTrackerRequest(Session& session, const TorrentSP& torrent,
										const PpsBaseinfoRetriverSP& retriver, 
										const EndPoint& traker_address, TimeoutHandler handler);
																
private:
	virtual SocketConnectionSP CreateConnection() override;
	virtual bool ConstructSendRequest(SendBuffer& send_buf) override;
	virtual bool ProcessResponse(const char* buf, uint64 len) override;
	virtual void TickProc() override;
	virtual bool IsTimeout() override;

private:
	bool SendBaseinfoRequest(SendBuffer& send_buf);
	
	void ParseBaseinfoResp(const char* buf, uint64 len, PpsBaseInfoMsg& msg);
	void ProcBaseinfoResp(const char* buf, uint64 len);

private:
	Session& session_;
	TorrentWP torrent_;
	PpsBaseinfoRetriverSP baseinfo_retriver_;
	
	uint32 alive_time_;
	uint32 send_time_;

	uint32 seq_id_;
};

}
#endif 


