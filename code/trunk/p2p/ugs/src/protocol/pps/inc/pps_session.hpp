/*#############################################################################
 * �ļ���   : pps_session.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��23��
 * �ļ����� : PpsSession������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEADER_PPS_SESSION
#define HEADER_PPS_SESSION

#include "bt_types.hpp"
#include "session.hpp"
#include "endpoint.hpp"
#include "tracker_manager.hpp"
#include "pps_pkt_statistics.hpp"

namespace BroadCache
{
class PpsKeepAliveMsg;
class PpsReportResourceMsg;
class PpsReportServiceAddressMsg;
class PpsRequestProxyMsg;
class PpsRequestTrackerMsg;

/******************************************************************************
 * ������PpsЭ��Ự����
 * ���ߣ�tom_liu
 * ʱ�䣺2013��12��28��
 *****************************************************************************/
class PpsSession : public Session
{
public:
	PpsSession(boost::asio::io_service& ios);

	virtual bool Init();

	// ��ȡ��Ӧ��TrackerManager
	TrackerManager& GetTrackerManager();

	// ��ȡbt���ݰ���ͳ����Ϣ
    PpsPktStatistics& GetPktStatistics();
	
private:	
	virtual std::string GetPktFilter() override;
	
	void ProcessPkt(const Packet& pkt) override;

	virtual void InitPacketProcessor() override;

	void OnKeepAliveMsg(const PpsKeepAliveMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

	// �ϱ���Դ��Ϣ����
	void OnReportResouceMsg(const PpsReportResourceMsg& msg, const EndPoint& endpoint, 
										msg_seq_t seq);

	// �ϱ�rcs�����ַ��Ϣ����
	void OnReportServiceAddressMsg(const PpsReportServiceAddressMsg& msg, 
										const EndPoint& endpoint, msg_seq_t seq);


	void OnRequestTrackerMsg(const PpsRequestTrackerMsg& msg,
	                                    const EndPoint& endpoint, msg_seq_t seq);

private:
	bool all_redirect_;
	bool hangup_outer_peer_;
	std::string domain_;  // UGS���ڵ���

    TrackerManager tracker_manager_;  // TrackerManager
   
	PktProcessorSP pkt_processor_;  // pps���ݰ�������
	PpsPktStatistics pkt_statistics_; //pps���ݰ�ͳ����
};

}
#endif

