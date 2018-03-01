/*#############################################################################
 * �ļ���   : pps_session.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��23��
 * �ļ����� : BtSession������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_session.hpp"
#include "ugs_config_parser.hpp"
#include "message.pb.h"
#include "communicator.hpp"
#include "bc_util.hpp"
#include "ugs_util.hpp"
#include "tracker_manager.hpp"
#include "bc_assert.hpp"
#include "pps_pkt_processor.hpp"
#include "pps_pkt_statistics.hpp"
#include "message.pb.h"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * ��  ��: PpsSession��Ĺ��캯��
 * ��  ��: [in][out] ios IO�������
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��24��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsSession::PpsSession(boost::asio::io_service& ios)
	: Session(boost::bind(&PpsSession::ProcessPkt, this, _1), ios),
	  all_redirect_(false),
	  hangup_outer_peer_(false),
	  pkt_statistics_(ios)
{
	//��ȡ�����ļ����״̬��Ϣ
    GET_UGS_CONFIG_BOOL("pps.common.all-redirect", all_redirect_);
    GET_UGS_CONFIG_BOOL("pps.policy.hangup-outer-peer", hangup_outer_peer_);
    GET_UGS_CONFIG_STR("global.common.domain", domain_);

	//���communicator��������ش�����
    auto& communicator = Communicator::instance();
	communicator.RegisterMsgHandler<PpsKeepAliveMsg>(
        boost::bind(&PpsSession::OnKeepAliveMsg, this, _1, _2, _3));
	communicator.RegisterMsgHandler<PpsReportResourceMsg>(
        boost::bind(&PpsSession::OnReportResouceMsg, this, _1, _2, _3));
    communicator.RegisterMsgHandler<PpsReportServiceAddressMsg>(
        boost::bind(&PpsSession::OnReportServiceAddressMsg, this, _1, _2, _3));
    communicator.RegisterMsgHandler<PpsRequestTrackerMsg>(
        boost::bind(&PpsSession::OnRequestTrackerMsg, this, _1, _2, _3));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ʼ��bt���ݰ�������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.23
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsSession::InitPacketProcessor()
{
	BC_ASSERT(!pkt_processor_);

    auto this_ptr = boost::dynamic_pointer_cast<PpsSession>(shared_from_this());

	pkt_processor_ = PktProcessorSP(new PpsGetPeerProcessor(this_ptr));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ʼ��session
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsSession::Init()
{
    InitPacketProcessor(); 
    return Session::Init();
}


/*------------------------------------------------------------------------------
 * ��  ��: ��ȡpps���ݰ�ͳ����Ϣ
 * ��  ��:
 * ����ֵ: pps���ݰ�ͳ����Ϣ
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
PpsPktStatistics& PpsSession::GetPktStatistics()
{
    return pkt_statistics_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡtracker������ 
 * ��  ��:
 * ����ֵ: tracker������
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
TrackerManager& PpsSession::GetTrackerManager()
{
	return tracker_manager_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡpps���ݰ��İ�������򣨰���������
 * ��  ��:
 * ����ֵ: pps���ݰ��Ĳ������
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::string PpsSession::GetPktFilter()
{
	//get peerƥ�� 
	std::string get_peer_feature = "(udp[10:4] = 0x434a71FF)";
										
	//����ƥ��
	//std::string transfer_feature = "(udp[10:4] = 0x430000D8)";

	//handshakeƥ��
	//std::string handshake_feature = "(udp[10:4] = 0x43000082)";

	//meta��Ϣ����ƥ��
	//std::string basic_info_feature = "(udp[10:4] = 0x430000ED)";

	//������Ϣ����ƥ��
	//std::string base_info_feature = "(udp[10:4] = 0x434871FF)";

	std::string link_feature = " or ";

	std::string final_feature = get_peer_feature;

	//LOG(INFO) << "PPS Filter String | featur " << final_feature;
	return final_feature;
}

/*------------------------------------------------------------------------------
 * ��  ��: �������pps���ݰ�
 * ��  ��: [in] pkt ��������ݰ�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 	
void PpsSession::ProcessPkt(const Packet& pkt)
{
	LOG(INFO) << "Process Pps Packet";
	pkt_processor_->Handle(pkt.buf, pkt.len);

}

/*------------------------------------------------------------------------------
 * ��  ��: ������յ���rcs keep-alive��Ϣ
 * ��  ��: [in] msg ��Ϣ����
 *         [in] endpoint ���ʹ���Ϣ��rcs��ַ
 *         [in] seq ����Ϣ��Ӧ�����к�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnKeepAliveMsg(const PpsKeepAliveMsg& msg, 
                               const EndPoint& endpoint,
                               msg_seq_t /*seq*/)
{	
    LOG(INFO) << "Receive Pps keep-alive message from service node : " << endpoint;
	
    service_node_detector().OnKeepAliveMsg(endpoint);
}

/*------------------------------------------------------------------------------
 * ��  ��: ������յ���rcs�ϱ���Դ��Ϣ
 * ��  ��: [in] msg ��Ϣ����
 *         [in] endpoint ���ʹ���Ϣ��rcs��ַ
 *         [in] seq ����Ϣ��Ӧ�����к�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnReportResouceMsg(const PpsReportResourceMsg& msg, 
                                   const EndPoint& endpoint,
                                   msg_seq_t /*seq*/)
{
    LOG(INFO) << "Receive Pps report file resouce message from service node: " << endpoint;

    std::vector<std::string> info_hash;
    for (int i = 0; i < msg.info_hash_size(); ++i)
    {
        info_hash.push_back(msg.info_hash(i));
        LOG(INFO) << ToHex(msg.info_hash(i));
    }

    rcs_mapper().AddFileResource(endpoint, info_hash);
}

/*------------------------------------------------------------------------------
 * ��  ��: ������յ���rcs �ϱ������ַ��Ϣ
 * ��  ��: [in] msg ��Ϣ����
 *         [in] endpoint ���ʹ���Ϣ��rcs��ַ
 *         [in] seq ����Ϣ��Ӧ�����к�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnReportServiceAddressMsg(const PpsReportServiceAddressMsg& msg,
                                          const EndPoint& endpoint,
                                          msg_seq_t seq)
{
	LOG(INFO) << "Receive Pps report service address message from service node : " << endpoint;

    std::vector<EndPoint> rcs;
    for (int i = 0; i < msg.rcs_size(); ++i)
    {
        auto& ep = msg.rcs(i);
        rcs.push_back(EndPoint(ep.ip(), static_cast<uint16>(ep.port())));

        LOG(INFO) << rcs.back();
    }

    rcs_mapper().AddRcsResource(endpoint, rcs);
}

/*------------------------------------------------------------------------------
 * ��  ��: ������յ���rcs ����tracker�б���Ϣ
 * ��  ��: [in] msg ��Ϣ����
 *         [in] endpoint ���ʹ���Ϣ��rcs��ַ
 *         [in] seq ����Ϣ��Ӧ�����к�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnRequestTrackerMsg(const PpsRequestTrackerMsg& msg,
                                    const EndPoint& endpoint,
                                    msg_seq_t seq)
{
	//�����û��tracker��ַ�� �о͹�����Ӧ���Ĳ�����

    LOG(INFO) << "Received pps request tracker message from service node : " << endpoint;

    TrackerManager* tracker_manager = nullptr;

    TrackerType tracker_type = msg.tracker_type();

    if (tracker_type == UDP_TRACKER)
    {
        tracker_manager = &tracker_manager_;
    }

    if (tracker_manager == nullptr)
        return ;

    LOG(INFO) << "Info-hash : " << ToHex(msg.info_hash());

    PpsReplyTrackerMsg reply_msg;
    reply_msg.set_domain(msg.domain());
    reply_msg.set_tracker_type(msg.tracker_type());
    reply_msg.set_info_hash(msg.info_hash());

    LOG(INFO) << "Reply tracker-list : ";

    std::vector<EndPoint> trackers = tracker_manager->GetTrackers(msg.info_hash());
    FOREACH(e, trackers)
    {
        EndPointStruct* tracker = reply_msg.add_tracker();
        tracker->set_ip(e.ip);
        tracker->set_port(e.port); 
        LOG(INFO) << e;
    }

    Communicator::instance().SendMsg(reply_msg, endpoint, seq);
}

}