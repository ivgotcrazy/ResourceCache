/*#############################################################################
 * �ļ���   : pps_tracker_request.cpp
 * ������   : tom_liu
 * ����ʱ�� : 2014��1��2��
 * �ļ����� : PpsTrackerRequestʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#include "pps_tracker_request.hpp"

#include "net_byte_order.hpp"
#include "pps_session.hpp"
#include "torrent.hpp"
#include "policy.hpp"
#include "ip_filter.hpp"
#include "pps_pub.hpp"
#include "bc_io.hpp"
#include "rcs_util.hpp"
#include "pps_torrent.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsTrackerRequest::PpsTrackerRequest(Session & session, const TorrentSP & torrent, 
												const EndPoint & traker_address) 
	: TrackerRequest(traker_address)
	, torrent_(torrent)
	, session_(session)
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
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
SocketConnectionSP PpsTrackerRequest::CreateConnection()
{
	SocketConnectionSP sock_conn (new UdpSocketConnection(session_.sock_ios(), tracker_address()));
	return sock_conn;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����send request ����
 * ��  ��: [out] send_buf   ���ͱ��Ļ�����
 * ����ֵ: bool true �����ɹ���false ʧ��
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsTrackerRequest::ConstructSendRequest(SendBuffer& send_buf)
{	
	SendPeerListRequest(send_buf);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����connect request ����
 * ��  ��: [out]  send_buf
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsTrackerRequest::SendPeerListRequest(SendBuffer& send_buf)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	LOG(INFO) << "Send Pps peer list request msg";
	
	send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);
	
	char* buffer = send_buf.buf;
	buffer = buffer + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_PEERLIST_REQ, buffer);

	//IO::write_uint32(seq_id_, buffer);
	IO::write_uint32_be(0x03, buffer);

	IO::write_uint8(0x14, buffer);
	std::memcpy(buffer, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
	buffer += 20;

	IO::write_uint32(0x00, buffer);
	IO::write_uint16(0x0c, buffer);

	char unkown_one[] = { 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00 };
	std::memcpy(buffer, unkown_one, sizeof(unkown_one));
	buffer += sizeof(unkown_one);

	//IO::write_uint32(GetLocalAddress().ip, buffer);
	IO::write_uint32(0xc0a800bf, buffer);
	IO::write_uint16(GetLocalAddress().port, buffer);

	IO::write_uint32_be(0x01, buffer);

	char unkown_two[] = { (char)0x0b, (char)0x00, (char)0xfc, (char)0x01 };
	char unkown_three[] = { (char)0xff, (char)0xff, (char)0x00, (char)0x00 };
	char unkown_four[] = { (char)0xff, (char)0xff, (char)0x00, (char)0x00 };
	std::memcpy(buffer, unkown_two, sizeof(unkown_two));
	buffer += sizeof(unkown_two);
	std::memcpy(buffer, unkown_three, sizeof(unkown_three));
	buffer += sizeof(unkown_three);
	std::memcpy(buffer, unkown_four, sizeof(unkown_four));
	buffer += sizeof(unkown_four);

	std::memset(buffer, 0, 12);  //0x00
	buffer += 12;
	
	IO::write_uint32(0x01, buffer);

	uint16 packet_len = buffer - send_buf.buf; //metadata���ü�4 
	char* front_pos = send_buf.buf;
	IO::write_uint16_be(packet_len, front_pos); //����ֱ����send_buf.buf,writeϵ�к������buffer��λ

	send_buf.len = packet_len;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ձ��ĵĽ����ӿ�
 * ��  ��: [in] buf ���ջ����� 
           [in] len ��������С
 * ����ֵ: bool true: �������ͱ��� false ���������ͱ���
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsTrackerRequest::ProcessResponse(const char* buf, uint64 len)
{	
	LOG(INFO) << "Recv tracker response ";
	//��ȡ����֮���3�ֽڣ�ƥ���ʶ�ֶ�
	const char* buffer;
	buffer = buf + 3;
	PpsMsgType msg_type = static_cast<PpsMsgType>(IO::read_uint32(buffer));

	if (msg_type != PPS_MSG_PEERLIST_RES) return false;
	ProcPeerListResp(buf, len);
	
	return false;
}

void PpsTrackerRequest::ParsePeerListResp(const char* buf, uint64 len, PpsPeerListMsg& msg)
{
	const char* priv_postion = buf;
	
    uint16 msg_length = IO::read_int16_be(buf);  
	
	buf += 5; //identify_id
	buf += 4; // 0x0b
	buf += 1 + 20; //hash_len + hash
	buf += 7; //0x00
	
	msg.peer_num = IO::read_uint16(buf);
	LOG(INFO) << "peer num  ------------- : " << msg.peer_num;

	buf += 3; // 0x0c 81 01

	for (uint32 i = 0; i < msg.peer_num && (uint16)(buf - priv_postion) < msg_length; i++)
	{
		buf += 1; //0x0b 

		EndPoint ep;
		ep.ip = IO::read_uint32(buf);
		ep.port = IO::read_uint16_be(buf);  //port��ַ�Ƿ����
		msg.peer_list.push_back(ep);

		buf += 2; //unkown
		buf += 3; //0x0c 81 01
	}

	//ʣ��δ֪�ֽ���δ����Ӱ�콻�����ݲ�����
	
}

void PpsTrackerRequest::ProcPeerListResp(const char* buf, uint64 len)
{
	LOG(INFO) << " Proc peer list response ";
	PpsPeerListMsg msg;
	ParsePeerListResp(buf, len, msg);
	
	//���ص�torrent��
	TorrentSP torrent =  torrent_.lock();
	if (!torrent) return;        // torrent error

	IpFilter& filter = IpFilter::GetInstance();
	for (EndPoint ep : msg.peer_list)
	{
		AccessInfo info = filter.Access(to_address(ep.ip));
		PeerType peer_type = (info.flags == INNER) ? PEER_INNER : PEER_OUTER;
		torrent->policy()->AddPeer(ep, peer_type, PEER_RETRIVE_TRACKER);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: tick ����ӿ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTrackerRequest::TickProc()
{
	alive_time_ ++;     
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƿ�ʱ
 * ��  ��:
 * ����ֵ: bool true: �ǳ�ʱ false :û�г�ʱ
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsTrackerRequest::IsTimeout()
{	
	if(!send_time_ && (alive_time_ - send_time_ >= TIMEOUT )) return true;
	return false;
}
	
}
