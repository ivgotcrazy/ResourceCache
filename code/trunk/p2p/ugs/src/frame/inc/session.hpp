/*#############################################################################
 * 文件名   : session.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : Session类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_SESSION
#define HEADER_SESSION

#include <string>
#include <queue>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include "bc_typedef.hpp"
#include "pkt_capturer.hpp"
#include "service_node_detector.hpp"
#include "rcs_mapper.hpp"
#include "ugs_typedef.hpp"
#include "hot_resource_manager.hpp"

namespace BroadCache
{
/******************************************************************************
 * 描述：Session
 * 作者：teck_zhou
 * 时间：2013年11月13日
 *****************************************************************************/
class Session : public boost::noncopyable,
                public boost::enable_shared_from_this<Session>
{
public:
	typedef boost::function<void(const Packet&)> ProcessPktHandler;

	Session(ProcessPktHandler handler, boost::asio::io_service& ios);
	virtual ~Session();

	virtual bool Init();

	bool Start();

	// 获取内网peer列表
    std::vector<EndPoint> GetLocalPeers(const std::string& info_hash);

    // 获取所有活跃的RCS资源
    std::vector<EndPoint> GetAllRcs();

	//判断一个资源是否是热点资源
	bool IsHotResource(const std::string& info_hash, const EndPoint& ep);

private:
	// 获取协议定义的报文过滤规则
	virtual std::string GetPktFilter() = 0;

	// 处理捕获的bt数据包
	virtual void ProcessPkt(const Packet& pkt) = 0;

	// 初始化数据包处理器
	virtual void InitPacketProcessor() = 0;

	// 服务结点超时
	void OnServiceNodeTimeout(const EndPoint& endpoint);

	// libpcap捕获报文后的处理
	void OnCapturePkt(const Packet& pkt);

	// 多线程报文处理函数
	void PktConsumerThreadFunc();

protected:
	RcsMapper& rcs_mapper() { return rcs_mapper_; }
	ServiceNodeDetector& service_node_detector() { return service_node_detector_; }
	
private:
	ProcessPktHandler pkt_handler_;
	boost::scoped_ptr<PktCapturer> pkt_capturer_;	

	std::queue<Packet> captured_pkts_;
	boost::mutex pkt_mutex_;
	boost::condition_variable pkt_condition_;

	std::vector<boost::thread> pkt_consume_threads_;

	ServiceNodeDetector service_node_detector_;  // 服务结点存活状态检测
	HotResourceManager	hot_resource_manager_;
	
	RcsMapper rcs_mapper_;  // rcs地址映射
	PeerPoolSP peer_pool_;  // 内网peer信息

    bool provide_inner_peer_;  // 是否提供内网peer    
    uint32 max_local_peer_num_ = 10;  // 提供内网最大数目
	uint32 peer_alive_time_ = 30 * 60;  // 内网peer的超时时间（单位:秒）
	
	bool stop_flag_;
};

}

#endif
