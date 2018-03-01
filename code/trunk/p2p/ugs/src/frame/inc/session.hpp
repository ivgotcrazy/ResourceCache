/*#############################################################################
 * �ļ���   : session.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : Session������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ������Session
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��11��13��
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

	// ��ȡ����peer�б�
    std::vector<EndPoint> GetLocalPeers(const std::string& info_hash);

    // ��ȡ���л�Ծ��RCS��Դ
    std::vector<EndPoint> GetAllRcs();

	//�ж�һ����Դ�Ƿ����ȵ���Դ
	bool IsHotResource(const std::string& info_hash, const EndPoint& ep);

private:
	// ��ȡЭ�鶨��ı��Ĺ��˹���
	virtual std::string GetPktFilter() = 0;

	// �������bt���ݰ�
	virtual void ProcessPkt(const Packet& pkt) = 0;

	// ��ʼ�����ݰ�������
	virtual void InitPacketProcessor() = 0;

	// �����㳬ʱ
	void OnServiceNodeTimeout(const EndPoint& endpoint);

	// libpcap�����ĺ�Ĵ���
	void OnCapturePkt(const Packet& pkt);

	// ���̱߳��Ĵ�����
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

	ServiceNodeDetector service_node_detector_;  // ��������״̬���
	HotResourceManager	hot_resource_manager_;
	
	RcsMapper rcs_mapper_;  // rcs��ַӳ��
	PeerPoolSP peer_pool_;  // ����peer��Ϣ

    bool provide_inner_peer_;  // �Ƿ��ṩ����peer    
    uint32 max_local_peer_num_ = 10;  // �ṩ���������Ŀ
	uint32 peer_alive_time_ = 30 * 60;  // ����peer�ĳ�ʱʱ�䣨��λ:�룩
	
	bool stop_flag_;
};

}

#endif
