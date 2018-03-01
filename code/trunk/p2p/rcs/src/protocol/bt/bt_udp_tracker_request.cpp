/*#############################################################################
 * �ļ���   : bt_udp_tracker_request.cpp
 * ������   : vincent_pan
 * ����ʱ�� : 2013��11��18��
 * �ļ����� : BtUdpTrackerRequestʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#include "bt_udp_tracker_request.hpp"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "net_byte_order.hpp"
#include "bt_session.hpp"
#include "torrent.hpp"
#include "policy.hpp"
#include "ip_filter.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtUdpTrackerRequest::BtUdpTrackerRequest(Session & session, const TorrentSP & torrent, const EndPoint & traker_address)
	: TrackerRequest(traker_address)
	, state_(TS_INIT)
	, torrent_(torrent)
	, session_(session)
	, connection_id_(0)
	, transaction_id_(0)
	, alive_time_(0)
	, send_time_(0)
{
	Session::ListenAddrVec addr_vec = session_.GetListenAddr();
	BC_ASSERT(!addr_vec.empty());
	local_address_ = addr_vec.front();   // local_address 
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����socket ����
 * ��  ��:
 * ����ֵ: SocketConnectionSP
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketConnectionSP BtUdpTrackerRequest::CreateConnection()
{
	SocketConnectionSP sock_conn (new UdpSocketConnection(session_.sock_ios(), tracker_address()));
	state_ = TS_INIT;
	return sock_conn;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����send request ����
 * ��  ��: [out] send_buf   ���ͱ��Ļ�����
 * ����ֵ: bool true �����ɹ���false ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtUdpTrackerRequest::ConstructSendRequest(SendBuffer& send_buf)
{	
	if (state_ == TS_INIT || state_ == TS_ERROR || 
		state_ == TS_CONNECTING || state_ == TS_OK)
	{
		SendConnectRequest(send_buf);
		return true;
	}
	else if (state_ == TS_ANNOUNCING )
	{
		if(SendAnnounceRequest(send_buf)) return true;

		return false;
	}
	else
	{
		return false;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����connect request ����
 * ��  ��: [out]  send_buf
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtUdpTrackerRequest::SendConnectRequest(SendBuffer& send_buf)
{
	boost::random::mt19937 gen;
	boost::random::uniform_int_distribution<> dist(1000, 0x0FFFFFFF);  // 1000 - 0x0FFFFFFF �������
	
	send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);  //���䷢�ͻ�����
	NetSerializer serialzer(send_buf.buf);
	transaction_id_ = dist(gen);
	serialzer & int64 (0x41727101980) 
		      & int32 (0) 
		      & int32 (transaction_id_);
	
	send_buf.len = serialzer.value() - send_buf.buf;
	send_time_ = alive_time_;
	state_ = TS_CONNECTING;
}

/*-----------------------------------------------------------------------------
 * ��  ��:���� announce request ���� 
 * ��  ��:[out] send_buf
 * ����ֵ: bool ture: �����ɹ� false : ����ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtUdpTrackerRequest::SendAnnounceRequest(SendBuffer& send_buf)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	boost::random::mt19937 gen;
	boost::random::uniform_int_distribution<> dist(1000, 0x0FFFFFFF);  // 1000 - 0x0FFFFFFF �������

	
	send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);
	NetSerializer serializer(send_buf.buf);
	transaction_id_ = dist(gen);
	serializer & int64(connection_id_)    // connection_id
		       & int32 (1)                // action_id
		       & int32 (transaction_id_); // transaction_id

	memcpy(serializer.value(), torrent->info_hash()->raw_data().c_str(), 20); //info_hash
	serializer.advance(20);
	memcpy(serializer.value(), BtSession::GetLocalPeerId(), 20);              //peer_id
	serializer.advance(20);

	serializer & int64 (100*100)                 //downloadsize
		       & int64 (0)                       //left
		       & int64 (100*100)                 //uploadsize
		       & int32 (0)                       //event
		       & uint32 (local_address_.ip)      //ip address
		       & uint32 (dist(gen))              //key
		       & int32 (-1)                      //num_want
		       & uint16 (local_address_.port)    //tcp port 
		       & uint16 (0);                     //extensions
	send_buf.len = serializer.value() - send_buf.buf;        // ���ĳ���
	send_time_ = alive_time_;
	state_ = TS_ANNOUNCING;
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ձ��ĵĽ����ӿ�
 * ��  ��: [in] buf  ���ջ����� 
           [in] len  ��������С
 * ����ֵ: bool true: �������ͱ��� false ���������ͱ���
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtUdpTrackerRequest::ProcessResponse(const char* buf, uint64 len)
{	
	if (len < 16) return false;   // msg error

	uint32 action_id;
	uint32 transaction_id;
	NetUnserializer unserialize(buf);
	unserialize & action_id
	            & transaction_id;
		
	if (transaction_id != transaction_id_) return false;   // msg error
	
	if (ProcessPkg(action_id, buf+8, len- 8)) return true;  //receive connect responce, conutiue send msg. 

	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ձ��ĵľ��崦��ӿ�
 * ��  ��: [in] action_id
           [in] buf
           [in] len
 * ����ֵ: bool true : �������ͱ��� false : ���������ͱ���
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtUdpTrackerRequest::ProcessPkg(uint32 action_id, const char* buf, uint64 len)
{	
	if(action_id == 3)                             //error msg
	{
		state_ = TS_ERROR;
		return false;		              // error 
	}
	else if (action_id == 0)                       // connect responce msg
	{
		NetToHost<int64>(buf, connection_id_);
		state_ = TS_ANNOUNCING;
		return true;                       // conutiue sending msg
	}
	else if (action_id == 1)                       // announce responce msg
	{	
		if(len < 12) 
		{
			LOG(INFO)<< "Bad announce responce msg.";
			return false;                  // msg error
		}

		TorrentSP torrent = torrent_.lock();
		if (!torrent) return false;        // torrent error

		IpFilter& filter = IpFilter::GetInstance();

		char* pbuf = NULL;
		for(unsigned int i=0; i<(len-12)/6; ++i)
		{
			pbuf = const_cast<char *>(buf) + 12 + i*6;
			uint32 ip = 0;  
			NetToHost<uint32>(pbuf, ip);
			uint16 port = 0;
			NetToHost<uint16>(pbuf+4, port);
			EndPoint end_point(ip, port);

			AccessInfo info = filter.Access(to_address(ip));
			PeerType peer_type = (info.flags == INNER) ? PEER_INNER : PEER_OUTER;

			torrent->policy()->AddPeer(end_point, peer_type, PEER_RETRIVE_TRACKER);
		}
		state_ = TS_OK;
		LOG(INFO)<< "Success to receive announce responce msg.";
		return false;                       // complete 
	}
	else 
	{
		return false;                       // error
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: tick ����ӿ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtUdpTrackerRequest::TickProc()
{
	alive_time_ ++;     
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƿ�ʱ
 * ��  ��:
 * ����ֵ: bool true: �ǳ�ʱ false :û�г�ʱ
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtUdpTrackerRequest::IsTimeout()
{	
	if(!send_time_ && (alive_time_ - send_time_ >= TIMEOUT )) return true;
	return false;
}
	
}
