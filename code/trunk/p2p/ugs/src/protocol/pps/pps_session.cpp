/*#############################################################################
 * 文件名   : pps_session.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月23日
 * 文件描述 : BtSession类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描  述: PpsSession类的构造函数
 * 参  数: [in][out] ios IO服务对象
 * 返回值: 
 * 修  改:
 *   时间 2013年12月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PpsSession::PpsSession(boost::asio::io_service& ios)
	: Session(boost::bind(&PpsSession::ProcessPkt, this, _1), ios),
	  all_redirect_(false),
	  hangup_outer_peer_(false),
	  pkt_statistics_(ios)
{
	//读取配置文件相关状态信息
    GET_UGS_CONFIG_BOOL("pps.common.all-redirect", all_redirect_);
    GET_UGS_CONFIG_BOOL("pps.policy.hangup-outer-peer", hangup_outer_peer_);
    GET_UGS_CONFIG_STR("global.common.domain", domain_);

	//填充communicator，加载相关处理函数
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
 * 描  述: 初始化bt数据包处理器
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.23
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsSession::InitPacketProcessor()
{
	BC_ASSERT(!pkt_processor_);

    auto this_ptr = boost::dynamic_pointer_cast<PpsSession>(shared_from_this());

	pkt_processor_ = PktProcessorSP(new PpsGetPeerProcessor(this_ptr));
}

/*------------------------------------------------------------------------------
 * 描  述: 初始化session
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool PpsSession::Init()
{
    InitPacketProcessor(); 
    return Session::Init();
}


/*------------------------------------------------------------------------------
 * 描  述: 获取pps数据包统计信息
 * 参  数:
 * 返回值: pps数据包统计信息
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
PpsPktStatistics& PpsSession::GetPktStatistics()
{
    return pkt_statistics_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取tracker管理器 
 * 参  数:
 * 返回值: tracker管理器
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TrackerManager& PpsSession::GetTrackerManager()
{
	return tracker_manager_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取pps数据包的包捕获规则（包过滤器）
 * 参  数:
 * 返回值: pps数据包的捕获规则
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::string PpsSession::GetPktFilter()
{
	//get peer匹配 
	std::string get_peer_feature = "(udp[10:4] = 0x434a71FF)";
										
	//传输匹配
	//std::string transfer_feature = "(udp[10:4] = 0x430000D8)";

	//handshake匹配
	//std::string handshake_feature = "(udp[10:4] = 0x43000082)";

	//meta信息请求匹配
	//std::string basic_info_feature = "(udp[10:4] = 0x430000ED)";

	//基本信息请求匹配
	//std::string base_info_feature = "(udp[10:4] = 0x434871FF)";

	std::string link_feature = " or ";

	std::string final_feature = get_peer_feature;

	//LOG(INFO) << "PPS Filter String | featur " << final_feature;
	return final_feature;
}

/*------------------------------------------------------------------------------
 * 描  述: 处理捕获的pps数据包
 * 参  数: [in] pkt 捕获的数据包
 * 返回值:
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 	
void PpsSession::ProcessPkt(const Packet& pkt)
{
	LOG(INFO) << "Process Pps Packet";
	pkt_processor_->Handle(pkt.buf, pkt.len);

}

/*------------------------------------------------------------------------------
 * 描  述: 处理接收到的rcs keep-alive消息
 * 参  数: [in] msg 消息对象
 *         [in] endpoint 发送此消息的rcs地址
 *         [in] seq 此消息对应的序列号
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnKeepAliveMsg(const PpsKeepAliveMsg& msg, 
                               const EndPoint& endpoint,
                               msg_seq_t /*seq*/)
{	
    LOG(INFO) << "Receive Pps keep-alive message from service node : " << endpoint;
	
    service_node_detector().OnKeepAliveMsg(endpoint);
}

/*------------------------------------------------------------------------------
 * 描  述: 处理接收到的rcs上报资源消息
 * 参  数: [in] msg 消息对象
 *         [in] endpoint 发送此消息的rcs地址
 *         [in] seq 此消息对应的序列号
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 处理接收到的rcs 上报服务地址消息
 * 参  数: [in] msg 消息对象
 *         [in] endpoint 发送此消息的rcs地址
 *         [in] seq 此消息对应的序列号
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 处理接收到的rcs 请求tracker列表消息
 * 参  数: [in] msg 消息对象
 *         [in] endpoint 发送此消息的rcs地址
 *         [in] seq 此消息对应的序列号
 * 返回值:
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnRequestTrackerMsg(const PpsRequestTrackerMsg& msg,
                                    const EndPoint& endpoint,
                                    msg_seq_t seq)
{
	//检测有没有tracker地址， 有就构造相应报文并发送

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