/*#############################################################################
 * �ļ���   : bt_session.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : BtSession������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BT_SESSION
#define HEADER_BT_SESSION

#include "bt_types.hpp"
#include "session.hpp"
#include "endpoint.hpp"
#include "tracker_manager.hpp"
#include "bt_packet_statistics.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

class BtKeepAliveMsg;
class BtReportResourceMsg;
class BtReportServiceAddressMsg;
class BtRequestProxyMsg;
class BtRequestTrackerMsg;

/******************************************************************************
 * ������BTЭ��Ự����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��11��13��
 *****************************************************************************/
class BtSession : public Session
{
public:
	explicit BtSession(boost::asio::io_service& ios);

    // ��ʼ��session
    virtual bool Init() override;

    // ��ȡhttp��Ӧ��TrackerManager
    TrackerManager& GetHttpTrackerManager();

    // ��ȡudp��Ӧ��TrackerManager
    TrackerManager& GetUdpTrackerManager();

    // ��ȡbt���ݰ���ͳ����Ϣ
    BtPacketStatistics& GetPacketStatistics();

private:
    // ��ȡbt���ݰ��Ĺ�����
	virtual std::string GetPktFilter() override;

    // �������bt���ݰ�
	virtual void ProcessPkt(const Packet& pkt) override;

	virtual void InitPacketProcessor() override;

	void OnKeepAliveMsg(const BtKeepAliveMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

	// �ϱ���Դ��Ϣ����
	void OnReportResouceMsg(const BtReportResourceMsg& msg, const EndPoint& endpoint, 
										msg_seq_t seq);

	// �ϱ�rcs�����ַ��Ϣ����
	void OnReportServiceAddressMsg(const BtReportServiceAddressMsg& msg, 
										const EndPoint& endpoint, msg_seq_t seq);
	
    // ����rcs��Ϣ����
    void OnRequestProxyMsg(const BtRequestProxyMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

    // ����tracker�б���Ϣ����
    void OnRequestTrackerMsg(const BtRequestTrackerMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

private:
    bool all_redirect_ = true;  // ����
    bool support_proxy_request_ = true;  // �Ƿ�֧��rcs��������
    bool hangup_outer_peer_ = true;  // �Ƿ��������peer����
    std::string resource_dispatch_policy_;  // ��Դ�ַ�����
    std::string domain_;  // UGS���ڵ���

    BtPacketStatistics packet_statistics_;  // bt���ݰ�ͳ����Ϣ

    TrackerManager http_tracker_manager_;  // http get��ʽ��TrackerManager
    TrackerManager udp_tracker_manager_;  // udp ��ʽ��TrackerManager
    
    PktProcessorSP pkt_processor_;  // bt���ݰ�������
};

}  // namespace BroadCache

#endif  // HEADER_BT_SESSION
