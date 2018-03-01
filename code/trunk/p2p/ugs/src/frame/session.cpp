/*#############################################################################
 * 文件名   : session.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : Session类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "session.hpp"
#include "bc_assert.hpp"
#include "communicator.hpp"
#include "mem_buf_pool.hpp"
#include "ugs_config_parser.hpp"
#include "ugs_util.hpp"
#include "peer_pool.hpp"
#include "message.pb.h"

namespace BroadCache
{

static const uint8 kPktConsumeThreadNum = 1;

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Session::Session(ProcessPktHandler handler, boost::asio::io_service& ios) 
	: pkt_handler_(handler), 
	  service_node_detector_(ios),
	  provide_inner_peer_(false),
	  stop_flag_(false)
{
	BC_ASSERT(pkt_handler_);
	
    // 读取配置信息  
	GET_UGS_CONFIG_BOOL("global.common.provide-local-peer", provide_inner_peer_);
	GET_UGS_CONFIG_INT("global.common.peer_alive_time_", peer_alive_time_);
    GET_UGS_CONFIG_INT("global.common.max-local-peers-num", max_local_peer_num_);

	service_node_detector_.SetTimeoutHandler(
        boost::bind(&Session::OnServiceNodeTimeout, this, _1));
        
    peer_pool_.reset(new PeerPool(ios, peer_alive_time_));

}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Session::~Session()
{
	stop_flag_ = true;

	for (boost::thread& thread : pkt_consume_threads_)
	{
		thread.join();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Session::Init()
{
	// 创建报文处理消费者线程
	for (uint8 i = 0; i < kPktConsumeThreadNum; i++)
	{
		pkt_consume_threads_.push_back(boost::thread(
			boost::bind(&Session::PktConsumerThreadFunc, this)));
	}

	// 创建并初始化报文捕获器
	pkt_capturer_.reset(new PktCapturer(GetPktFilter(), 
		boost::bind(&Session::OnCapturePkt, this, _1)));

	if (!pkt_capturer_->Init())
	{
		LOG(ERROR) << "Fail to init packet capturer";
		return false;
	}

	if (!hot_resource_manager_.Init())
	{
		LOG(ERROR) << "Fail to init hot resource manager";
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动报文捕获
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Session::Start()
{
	if (!pkt_capturer_->Start())
	{
		LOG(ERROR) << "Fail to start packet capturer";
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: libpcap捕获报文后的处理
 * 参  数: [in] pkt 报文
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Session::OnCapturePkt(const Packet& pkt)
{
	boost::mutex::scoped_lock lock(pkt_mutex_);
	captured_pkts_.push(pkt);
	pkt_condition_.notify_one();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取本地的peer列表
 * 参  数: [in] info_hash 下载文件的info_hash值
 * 返回值: peer列表
 * 修  改:
 *   时间 2013.11.22
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> Session::GetLocalPeers(const std::string& info_hash)
{
    std::vector<EndPoint> peer_list;
    if (provide_inner_peer_)
    {
        auto peers = peer_pool_->GetLocalPeers(info_hash, max_local_peer_num_);
        peer_list.swap(peers);  // 添加内网peer
    }

    // 添加根据info-hash映射到的rcs地址
    auto mapped_peers = rcs_mapper_.GetPeer(info_hash);
    peer_list.insert(peer_list.end(), mapped_peers.begin(), mapped_peers.end());

    return peer_list;
}

/*------------------------------------------------------------------------------
 * 描  述: 处理接收到的rcs keep-alive消息
 * 参  数: [in] endpoint 发送此消息的rcs地址
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void Session::OnServiceNodeTimeout(const EndPoint& endpoint)
{
    LOG(INFO) << "Receive service node timeout message : " << endpoint;
    rcs_mapper_.RemoveServiceNode(endpoint);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取所有活跃的RCS列表
 * 参  数:
 * 返回值: 所有活跃的RCS列表
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> Session::GetAllRcs()
{
    return rcs_mapper_.GetAllRcs();
}

/*-----------------------------------------------------------------------------
 * 描  述: 多线程报文处理函数
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Session::PktConsumerThreadFunc()
{
	while (!stop_flag_)
	{
		Packet pkt;
		{
			boost::mutex::scoped_lock lock(pkt_mutex_);
			while (captured_pkts_.empty())
			{
				pkt_condition_.wait(lock);
			}

			pkt = captured_pkts_.front();
			captured_pkts_.pop();
		}

		// 处理报文
		pkt_handler_(pkt);

		// 释放报文内存
		MemBufPool::GetInstance().FreePktBuf(pkt.buf);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 判断是否是热点资源 
 * 参  数: [in] info_hash 哈希
 * 		 [in] ep 地址
 * 返回值: true 是热点资源
 *         false 不是热点资源
 * 修  改:
 *   时间 2014年2月14日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Session::IsHotResource(const std::string& info_hash, const EndPoint& ep)
{
	ResourceId resource_id;
	resource_id.protocol = PROTOCOL_UNKNOWN;
	resource_id.info_hash = ToHex(info_hash);
	AccessRecord record;
	record.resource = resource_id;
	record.peer = ep;
	record.access_time = time(nullptr);

	return hot_resource_manager_.IsHotResource(record);
}

}
