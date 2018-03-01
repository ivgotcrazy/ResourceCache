/*#############################################################################
 * 文件名   : bt_session.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : BtSession类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描述：BT协议会话处理
 * 作者：teck_zhou
 * 时间：2013年11月13日
 *****************************************************************************/
class BtSession : public Session
{
public:
	explicit BtSession(boost::asio::io_service& ios);

    // 初始化session
    virtual bool Init() override;

    // 获取http对应的TrackerManager
    TrackerManager& GetHttpTrackerManager();

    // 获取udp对应的TrackerManager
    TrackerManager& GetUdpTrackerManager();

    // 获取bt数据包的统计信息
    BtPacketStatistics& GetPacketStatistics();

private:
    // 获取bt数据包的过滤器
	virtual std::string GetPktFilter() override;

    // 处理捕获的bt数据包
	virtual void ProcessPkt(const Packet& pkt) override;

	virtual void InitPacketProcessor() override;

	void OnKeepAliveMsg(const BtKeepAliveMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

	// 上报资源消息处理
	void OnReportResouceMsg(const BtReportResourceMsg& msg, const EndPoint& endpoint, 
										msg_seq_t seq);

	// 上报rcs服务地址消息处理
	void OnReportServiceAddressMsg(const BtReportServiceAddressMsg& msg, 
										const EndPoint& endpoint, msg_seq_t seq);
	
    // 请求rcs消息处理
    void OnRequestProxyMsg(const BtRequestProxyMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

    // 请求tracker列表消息处理
    void OnRequestTrackerMsg(const BtRequestTrackerMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

private:
    bool all_redirect_ = true;  // 待定
    bool support_proxy_request_ = true;  // 是否支持rcs代理请求
    bool hangup_outer_peer_ = true;  // 是否挂起外网peer连接
    std::string resource_dispatch_policy_;  // 资源分发策略
    std::string domain_;  // UGS所在的域

    BtPacketStatistics packet_statistics_;  // bt数据包统计信息

    TrackerManager http_tracker_manager_;  // http get方式的TrackerManager
    TrackerManager udp_tracker_manager_;  // udp 方式的TrackerManager
    
    PktProcessorSP pkt_processor_;  // bt数据包处理器
};

}  // namespace BroadCache

#endif  // HEADER_BT_SESSION
