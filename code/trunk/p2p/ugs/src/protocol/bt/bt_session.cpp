/*#############################################################################
 * 文件名   : bt_session.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : BtSession类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bt_session.hpp"
#include "peer_pool.hpp"
#include "ugs_config_parser.hpp"
#include "bt_packet_processor.hpp"
#include "message.pb.h"
#include "communicator.hpp"
#include "bc_util.hpp"
#include "ugs_util.hpp"
#include "pkt_processor.hpp"
#include "bc_assert.hpp"
#include "message.pb.h"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: BtSession类的构造函数
 * 参  数: [in][out] ios IO服务对象
 * 返回值: 
 * 修  改:
 *   时间 2013年11月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
BtSession::BtSession(boost::asio::io_service& ios)
	: Session(boost::bind(&BtSession::ProcessPkt, this, _1), ios),
      packet_statistics_(ios)
{
    // 读取配置信息  
    GET_UGS_CONFIG_BOOL("bt.common.all-redirect", all_redirect_);
    GET_UGS_CONFIG_BOOL("bt.common.support-proxy-request", support_proxy_request_);
    GET_UGS_CONFIG_BOOL("bt.policy.hangup-outer-peer", hangup_outer_peer_);
    GET_UGS_CONFIG_STR("bt.policy.resource-dispatch-policy", resource_dispatch_policy_);
    GET_UGS_CONFIG_STR("global.common.domain", domain_);
    
    // 设置接收来自communicator的消息
    auto& communicator = Communicator::instance();
	communicator.RegisterMsgHandler<BtKeepAliveMsg>(
        boost::bind(&BtSession::OnKeepAliveMsg, this, _1, _2, _3));
    communicator.RegisterMsgHandler<BtReportResourceMsg>(
        boost::bind(&BtSession::OnReportResouceMsg, this, _1, _2, _3));
    communicator.RegisterMsgHandler<BtReportServiceAddressMsg>(
        boost::bind(&BtSession::OnReportServiceAddressMsg, this, _1, _2, _3));
    communicator.RegisterMsgHandler<BtRequestProxyMsg>(
        boost::bind(&BtSession::OnRequestProxyMsg, this, _1, _2, _3));
    communicator.RegisterMsgHandler<BtRequestTrackerMsg>(
        boost::bind(&BtSession::OnRequestTrackerMsg, this, _1, _2, _3));
}

/*------------------------------------------------------------------------------
 * 描  述: 初始化bt数据包处理器
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void BtSession::InitPacketProcessor()
{
	BC_ASSERT(!pkt_processor_);

    auto this_ptr = boost::dynamic_pointer_cast<BtSession>(shared_from_this());

	pkt_processor_ = PktProcessorSP(new BtHttpGetPeerProcessor(this_ptr));

	pkt_processor_->AppendProcessor(PktProcessorSP(new BtUdpGetPeerProcessor(this_ptr)));

	if (hangup_outer_peer_)
	{
		pkt_processor_->AppendProcessor(PktProcessorSP(new BtHandshakeProcessor(this_ptr)));
	}
	
	pkt_processor_->AppendProcessor(PktProcessorSP(new BtDhtGetPeerProcessor(this_ptr)));
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
bool BtSession::Init()
{
    InitPacketProcessor(); 
    return Session::Init();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取bt数据包的包捕获规则（包过滤器）
 * 参  数:
 * 返回值: bt数据包的捕获规则
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::string BtSession::GetPktFilter()
{
	//handshake 匹配  .BitTorrent 
	std::string handshake_feature = "(tcp[((tcp[12:1]&0xf0) >> 2):4] = 0x13426974 \
										and tcp[(((tcp[12:1]&0xf0) >> 2) + 4):4] = 0x546f7272 \
										and tcp[(((tcp[12:1]&0xf0) >> 2) + 8):4] = 0x656e7420)";
										
	//http announce 匹配 GET /announc
	std::string http_tracker_feature = "(tcp[((tcp[12:1]&0xf0) >> 2):4] = 0x47455420 \
										   and tcp[(((tcp[12:1]&0xf0) >> 2) + 4):4] = 0x2f616e6e \
										   and tcp[(((tcp[12:1]&0xf0) >> 2) + 8):4] = 0x6f756e63)";

	//udp tracker 匹配长度117或108字节 且 第16字节后4字节为0x00000001
	std::string udp_tracker_feature = "( (udp[4:2] = 0x00000075 or udp[4:2] = 0x0000006c) \
	                                       and udp[16:4] = 0x00000001)";

	std::string link_feature = " or ";

	std::string final_feature = handshake_feature + link_feature + http_tracker_feature 
									 + link_feature + udp_tracker_feature;

	return final_feature;
}

/*------------------------------------------------------------------------------
 * 描  述: 处理捕获的bt数据包
 * 参  数: [in] pkt 捕获的数据包
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void BtSession::ProcessPkt(const Packet& pkt)
{
    if (pkt_processor_->Handle(pkt.buf, pkt.len))
    {
        packet_statistics_.GetCounter(BT_COMMON_PACKET).Increase();  //增加统计信息
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 获取http tracker管理器 
 * 参  数:
 * 返回值: http tracker管理器
 * 修  改:
 *   时间 2013.11.21
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TrackerManager& BtSession::GetHttpTrackerManager()
{
    return http_tracker_manager_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取udp tracker管理器 
 * 参  数:
 * 返回值: udp tracker管理器
 * 修  改:
 *   时间 2013.11.21
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
TrackerManager& BtSession::GetUdpTrackerManager()
{
    return udp_tracker_manager_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取bt数据包统计信息
 * 参  数:
 * 返回值: bt数据包统计信息
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
BtPacketStatistics& BtSession::GetPacketStatistics()
{
    return packet_statistics_;
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
void BtSession::OnKeepAliveMsg(const BtKeepAliveMsg& msg, 
                               const EndPoint& endpoint,
                               msg_seq_t /*seq*/)
{
    LOG(INFO) << "Receive Bt keep-alive message from service node : " << endpoint;
	
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
void BtSession::OnReportResouceMsg(const BtReportResourceMsg& msg, 
                                   const EndPoint& endpoint,
                                   msg_seq_t /*seq*/)
{	
    LOG(INFO) << "Receive Bt report file resouce message from service node: " << endpoint;

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
void BtSession::OnReportServiceAddressMsg(const BtReportServiceAddressMsg& msg,
                                          const EndPoint& endpoint,
                                          msg_seq_t seq)
{	
    LOG(INFO) << "Receive Bt report service address message from service node : " << endpoint;

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
 * 描  述: 处理接收到的rcs 请求rcs代理消息
 * 参  数: [in] msg 消息对象
 *         [in] endpoint 发送此消息的rcs地址
 *         [in] seq 此消息对应的序列号
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void BtSession::OnRequestProxyMsg(const BtRequestProxyMsg& msg, 
                                  const EndPoint& endpoint,
                                  msg_seq_t seq)
{
    LOG(INFO) << "Receive request rcs proxy message from service node : " << endpoint;
    LOG(INFO) << "Info-hash : " << ToHex(msg.info_hash());

    if (!support_proxy_request_)
    {
        LOG(WARNING) << "Proxy request is not support, but request proxy message recieved.";
        return ;
    }

    if (msg.rcs_type() != INNER_PROXY)
    {
        LOG(WARNING) << "Proxy request type is not supported.";
        return ;
    }

    BtReplyProxyMsg reply_msg;
    reply_msg.set_domain(msg.domain());
    reply_msg.set_rcs_type(msg.rcs_type());
    reply_msg.set_info_hash(msg.info_hash());

    LOG(INFO) << "Reply rcs proxy : ";

    auto rcs = rcs_mapper().GetRcsProxy(endpoint, msg.info_hash(), msg.num_want());
    FOREACH(e, rcs)
    {
        EndPointStruct* proxy = reply_msg.add_proxy();
        proxy->set_ip(e.ip);
        proxy->set_port(e.port);
        LOG(INFO) << e;
    }

    Communicator::instance().SendMsg(reply_msg, endpoint, seq);
}

/*------------------------------------------------------------------------------
 * 描  述: 处理接收到的rcs 请求tracker列表消息
 * 参  数: [in] msg 消息对象
 *         [in] endpoint 发送此消息的rcs地址
 *         [in] seq 此消息对应的序列号
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void BtSession::OnRequestTrackerMsg(const BtRequestTrackerMsg& msg,
                                    const EndPoint& endpoint,
                                    msg_seq_t seq)
{
    LOG(INFO) << "Received request tracker message from service node : " << endpoint;

    TrackerManager* tracker_manager = nullptr;

    TrackerType tracker_type = msg.tracker_type();
    if (tracker_type == HTTP_TRACKER)
    {
        tracker_manager = &http_tracker_manager_;
    }
    else if (tracker_type == UDP_TRACKER)
    {
        tracker_manager = &udp_tracker_manager_;
    }

    if (tracker_manager == nullptr)
        return ;

    LOG(INFO) << "Info-hash : " << ToHex(msg.info_hash());

    BtReplyTrackerMsg reply_msg;
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
