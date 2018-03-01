/*#############################################################################
 * 文件名   : pps_session.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月23日
 * 文件描述 : PpsSession类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描述：Pps协议会话处理
 * 作者：tom_liu
 * 时间：2013年12月28日
 *****************************************************************************/
class PpsSession : public Session
{
public:
	PpsSession(boost::asio::io_service& ios);

	virtual bool Init();

	// 获取对应的TrackerManager
	TrackerManager& GetTrackerManager();

	// 获取bt数据包的统计信息
    PpsPktStatistics& GetPktStatistics();
	
private:	
	virtual std::string GetPktFilter() override;
	
	void ProcessPkt(const Packet& pkt) override;

	virtual void InitPacketProcessor() override;

	void OnKeepAliveMsg(const PpsKeepAliveMsg& msg, const EndPoint& endpoint, msg_seq_t seq);

	// 上报资源消息处理
	void OnReportResouceMsg(const PpsReportResourceMsg& msg, const EndPoint& endpoint, 
										msg_seq_t seq);

	// 上报rcs服务地址消息处理
	void OnReportServiceAddressMsg(const PpsReportServiceAddressMsg& msg, 
										const EndPoint& endpoint, msg_seq_t seq);


	void OnRequestTrackerMsg(const PpsRequestTrackerMsg& msg,
	                                    const EndPoint& endpoint, msg_seq_t seq);

private:
	bool all_redirect_;
	bool hangup_outer_peer_;
	std::string domain_;  // UGS所在的域

    TrackerManager tracker_manager_;  // TrackerManager
   
	PktProcessorSP pkt_processor_;  // pps数据包处理器
	PpsPktStatistics pkt_statistics_; //pps数据包统计器
};

}
#endif

