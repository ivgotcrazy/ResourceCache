/*#############################################################################
 * �ļ���   : session.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : Session��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Session::Session(ProcessPktHandler handler, boost::asio::io_service& ios) 
	: pkt_handler_(handler), 
	  service_node_detector_(ios),
	  provide_inner_peer_(false),
	  stop_flag_(false)
{
	BC_ASSERT(pkt_handler_);
	
    // ��ȡ������Ϣ  
	GET_UGS_CONFIG_BOOL("global.common.provide-local-peer", provide_inner_peer_);
	GET_UGS_CONFIG_INT("global.common.peer_alive_time_", peer_alive_time_);
    GET_UGS_CONFIG_INT("global.common.max-local-peers-num", max_local_peer_num_);

	service_node_detector_.SetTimeoutHandler(
        boost::bind(&Session::OnServiceNodeTimeout, this, _1));
        
    peer_pool_.reset(new PeerPool(ios, peer_alive_time_));

}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��14��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��ʼ��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Session::Init()
{
	// �������Ĵ����������߳�
	for (uint8 i = 0; i < kPktConsumeThreadNum; i++)
	{
		pkt_consume_threads_.push_back(boost::thread(
			boost::bind(&Session::PktConsumerThreadFunc, this)));
	}

	// ��������ʼ�����Ĳ�����
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
 * ��  ��: �������Ĳ���
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: libpcap�����ĺ�Ĵ���
 * ��  ��: [in] pkt ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::OnCapturePkt(const Packet& pkt)
{
	boost::mutex::scoped_lock lock(pkt_mutex_);
	captured_pkts_.push(pkt);
	pkt_condition_.notify_one();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ���ص�peer�б�
 * ��  ��: [in] info_hash �����ļ���info_hashֵ
 * ����ֵ: peer�б�
 * ��  ��:
 *   ʱ�� 2013.11.22
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> Session::GetLocalPeers(const std::string& info_hash)
{
    std::vector<EndPoint> peer_list;
    if (provide_inner_peer_)
    {
        auto peers = peer_pool_->GetLocalPeers(info_hash, max_local_peer_num_);
        peer_list.swap(peers);  // �������peer
    }

    // ��Ӹ���info-hashӳ�䵽��rcs��ַ
    auto mapped_peers = rcs_mapper_.GetPeer(info_hash);
    peer_list.insert(peer_list.end(), mapped_peers.begin(), mapped_peers.end());

    return peer_list;
}

/*------------------------------------------------------------------------------
 * ��  ��: ������յ���rcs keep-alive��Ϣ
 * ��  ��: [in] endpoint ���ʹ���Ϣ��rcs��ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void Session::OnServiceNodeTimeout(const EndPoint& endpoint)
{
    LOG(INFO) << "Receive service node timeout message : " << endpoint;
    rcs_mapper_.RemoveServiceNode(endpoint);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ���л�Ծ��RCS�б�
 * ��  ��:
 * ����ֵ: ���л�Ծ��RCS�б�
 * ��  ��:
 *   ʱ�� 2013.11.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> Session::GetAllRcs()
{
    return rcs_mapper_.GetAllRcs();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���̱߳��Ĵ�����
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
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

		// ������
		pkt_handler_(pkt);

		// �ͷű����ڴ�
		MemBufPool::GetInstance().FreePktBuf(pkt.buf);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ж��Ƿ����ȵ���Դ 
 * ��  ��: [in] info_hash ��ϣ
 * 		 [in] ep ��ַ
 * ����ֵ: true ���ȵ���Դ
 *         false �����ȵ���Դ
 * ��  ��:
 *   ʱ�� 2014��2��14��
 *   ���� tom_liu
 *   ���� ����
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
