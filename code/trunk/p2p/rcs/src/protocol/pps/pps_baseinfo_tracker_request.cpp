/*#############################################################################
 * �ļ���   : pps_baseinfo_tracker_request.cpp
 * ������   : tom_liu
 * ����ʱ�� : 2014��1��2��
 * �ļ����� : PpsBaseInfoTrackerRequestʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#include "pps_baseinfo_tracker_request.hpp"

#include "net_byte_order.hpp"
#include "pps_session.hpp"
#include "torrent.hpp"
#include "policy.hpp"
#include "ip_filter.hpp"
#include "pps_pub.hpp"
#include "bc_io.hpp"
#include "rcs_util.hpp"
#include "pps_torrent.hpp"
#include "pps_baseinfo_retriver.hpp"

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
PpsBaseinfoTrackerRequest::PpsBaseinfoTrackerRequest(Session& session,
																const TorrentSP& torrent,
																const PpsBaseinfoRetriverSP& retriver, 
															  	const EndPoint & traker_address, 
																TimeoutHandler handler)
	: TrackerRequest(traker_address)
	, session_(session)
	, torrent_(torrent)
	, baseinfo_retriver_(retriver)
	, alive_time_(0)
	, send_time_(0)
{

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
SocketConnectionSP PpsBaseinfoTrackerRequest::CreateConnection()
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
bool PpsBaseinfoTrackerRequest::ConstructSendRequest(SendBuffer& send_buf)
{	
	SendBaseinfoRequest(send_buf);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��:���� baseinfo ������ 
 * ��  ��:[out] send_buf
 * ����ֵ: bool ture: �����ɹ� false : ����ʧ��
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoTrackerRequest::SendBaseinfoRequest(SendBuffer& send_buf)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	LOG(INFO) << "Send Pps baseinfo request msg";
	
	send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	char* buffer = send_buf.buf;
	buffer += 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_INFO_TRACKER_REQ, buffer);

	IO::write_uint8(0x14, buffer);
	std::memcpy(buffer, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
	buffer += 20;

	IO::write_uint32(0x00, buffer);
	IO::write_uint32_be(0x09, buffer);
	IO::write_uint16(0x00, buffer);

	//���ڱ�׼��baseinfo������Ϣ��Ҫ���� pps_link_len, pps_link�����ֶΣ�
	//�����Բ��Ӵ����ֶ�ͬ�����Դﵽ��ȡmetadata_info��Ч��
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
bool PpsBaseinfoTrackerRequest::ProcessResponse(const char* buf, uint64 len)
{	
	LOG(INFO) << "Recv tracker response ";
	//��ȡ����֮���3�ֽڣ�ƥ���ʶ�ֶ�
	const char* buffer;
	buffer = buf + 3;
	PpsMsgType msg_type = static_cast<PpsMsgType>(IO::read_uint32(buffer));

	if (msg_type != PPS_MSG_INFO_TRACKER_RES) return false;

	ProcBaseinfoResp(buf, len);

	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Baseinfo��Ϣ
 * ��  ��: [in] buf ���ջ����� 
 *         [in] len ��������С
 *         [out] msg baseinfo��Ϣ 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsBaseinfoTrackerRequest::ParseBaseinfoResp(const char* buf, uint64 len, PpsBaseInfoMsg& msg)
{
	buf += 2; //packet_len
	buf += 5; //identify_id	
	buf += 1 + 20; // hash_len + hash  
	buf += 1 + 20; //hash_len + hash

	msg.bitfield_size = IO::read_uint32_be(buf);

	buf += 4; // unknown

	msg.piece_count = IO::read_uint32_be(buf);

	buf += 4; //mark
	buf += 4; //unknown
	buf += 4; //crc

	msg.metadata_size = IO::read_uint32_be(buf);

	buf += 4; //bitfiled checksum

	msg.file_size = IO::read_uint32_be(buf);

	//ʣ��31δ֪�ֽ���δ����Ӱ�콻�����ݲ�����
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Baseinfo��Ӧ
 * ��  ��: [in] buf ���ջ����� 
 *         [in] len ��������С
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2014��1��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsBaseinfoTrackerRequest::ProcBaseinfoResp(const char* buf, uint64 len)
{
	LOG(INFO) << "Proc base info response ";
	PpsBaseInfoMsg msg;
	ParseBaseinfoResp(buf, len, msg);
	
	//���ص�baseinfo��PpsBaseinfoRetriver��
	baseinfo_retriver_->AddBaseInfo(msg);
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
void PpsBaseinfoTrackerRequest::TickProc()
{
	if (IsTimeout())
	{
		TimeoutHandler();		
	}
	
	alive_time_++;     
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
bool PpsBaseinfoTrackerRequest::IsTimeout()
{	
	if (!send_time_ && (alive_time_ - send_time_ >= TIMEOUT )) return true;
	return false;
}
	
}

