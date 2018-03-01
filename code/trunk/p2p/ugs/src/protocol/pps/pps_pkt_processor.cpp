/*##############################################################################
 * �ļ���   : pps_packet_processor.cpp
 * ������   : tom_liu 
 * ����ʱ�� : 2013.12.23
 * �ļ����� : pps���ĵĴ������ʵ���ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "pps_pkt_processor.hpp"

#include <algorithm>

#include <boost/regex.hpp>

#include "raw_data.hpp"
#include "bc_util.hpp"
#include "bc_assert.hpp"
#include "net_byte_order.hpp"
#include "bencode_entry.hpp"
#include "bencode.hpp"
#include "pps_session.hpp"

namespace BroadCache
{
/*------------------------------------------------------------------------------
 * ��  ��: PpsGetPeerProcessor���캯��
 * ��  ��: [in] session PpsSession����ָ��
 * ����ֵ:
 * ��  ��:
 *	 ʱ�� 2013.12.27
 *	 ���� tom_liu
 *	 ���� ����
 -----------------------------------------------------------------------------*/
PpsGetPeerProcessor::PpsGetPeerProcessor(PpsSessionSP session)
	: pps_session_(session) 
{
	
}

/*------------------------------------------------------------------------------
 * ��  ��: ����peer��ȡpeer-list������
 * ��  ��: [in] data ���ݰ�������
 *         [in] length ���ݰ��ĳ���
 * ����ֵ: �����ݰ��Ƿ��Ѿ�����
 * ��  ��:
 *   ʱ�� 2013.12.27
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsGetPeerProcessor::Process(const void* data, uint32 length)
{

    LOG(INFO) << "PpsGetPeerProcessor Process(" << data << ", " << length << ")";

	//����data���ݣ� ��ȡpeer��Ϣ
	RawDataResolver resolver(data, length, RawDataResolver::FROM_ETHERNET);
	if (resolver.GetTransportProtocol() != IPPROTO_UDP) return false;

	char* udp_data = (char*)(resolver.GetTransportData());
	uint32 udp_data_length = resolver.GetTransportDataLength();
	LOG(INFO) << "udp data length : " << udp_data_length;

	if ((NetToHost<int32>(udp_data + 2)) != 0x434a71ff) return false; 

	LOG(INFO) << "Received udp get peer-list request.";    
	LOG(INFO) << "Source port : " << NetToHost<uint16>(resolver.GetTransportSourcePort());	 
	LOG(INFO) << "Dest port : " << NetToHost<uint16>(resolver.GetTransportDestPort());    
	LOG(INFO) << "info-hash : " << ToHex(std::string(udp_data + 12, udp_data + 32));

	std::string info_hash(udp_data + 12, udp_data + 32);
	EndPoint ep(NetToHost(resolver.GetIpHeader()->daddr), NetToHost(resolver.GetTransportDestPort()));

	//�洢��¼tracker��ַ
	pps_session_->GetTrackerManager().AddTracker(info_hash, ep);
	
	// �����ȵ���Դ�жϣ������ȵ���Դ�������ض���
	if (!pps_session_->IsHotResource(info_hash, ep)) return true;	
	LOG(INFO) << "Hot Resource redirect";

	pps_session_->GetPktStatistics().GetCounter(PPS_GET_PEER).Increase();
	
	//���췵�ذ�
	char buffer[1024];  // �ظ����ݻ�����	  
	RawDataConstructor constructor(buffer, data, length);	
	char* buf = (char*)(constructor.GetTransportData());
	uint32 response_len = 0;
	ConstructGetPeerResponse(udp_data, udp_data_length, buf, response_len);
	LOG(ERROR) << "response_len : " << response_len;
	constructor.SetTransportDataLength(response_len);

	RawDataSender::instance().Send(buffer, buf - buffer + response_len);
	
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����peer_list�������Ӧ��Ϣ
 * ��  ��: [in] udp_data �������ݵ�ַ
 *         [in] udp_data_length ����ĳ���
 *		  [out] buf �����ַ
 *         [out] len ���泤��
 * ����ֵ: �����ݰ��Ƿ��Ѿ�����
 * ��  ��:
 *   ʱ�� 2013.12.27
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsGetPeerProcessor::ConstructGetPeerResponse(const char* udp_data, const uint32 udp_data_length, 
																char* buf, uint32& len)
{

	//���֧�ֻ�ȡpeer��񣬷���֧�ֵ�peer����
	int peer_count = 1;
	uint16 peer_data_len = peer_count * 12;
	char* peer_ptr = new char[peer_data_len];

	//�ظ��ڵ�
	EndPoint peer(to_address_ul("192.168.0.191"), 8393);

	int8 peer_obj_len = 0x0b;
	const char peer_reserved[] = {(char)0x0c, (char)0x81, (char)0x01};
	char peer_reserved_middle[] = {(char)0x48, (char)0xc8};
	char *temp_ptr = peer_ptr;
	for (int i = 0; i < peer_count; ++i)
	{
		*((int8*)temp_ptr) = peer_obj_len;
		temp_ptr += 1;
		*((int32*)temp_ptr) = NetToHost<uint32>(peer.ip); //��Ҫ������
		temp_ptr += 4;
		*((int16*)temp_ptr) = (peer.port); //����ת��
		temp_ptr += 2;
		memcpy(temp_ptr, peer_reserved_middle, sizeof(peer_reserved_middle));
		temp_ptr += 2;
		memcpy(temp_ptr, peer_reserved, sizeof(peer_reserved));
		temp_ptr += 3;
	}

	char* pcontent = buf + 2;

	char identify_id[] = { (char)0x43, (char)0x4b, (char)0x17, (char)0xff, (char)0x00};
	memcpy(pcontent, identify_id, sizeof(identify_id));
	pcontent += 5;
	
	uint32 reservered_one = 0x0b;
	memcpy(pcontent, &reservered_one, sizeof(reservered_one));
	pcontent += 4;

	uint8 hash_len = 0x14;
	memcpy(pcontent, &hash_len, sizeof(hash_len));
	pcontent += 1;

	memcpy(pcontent, udp_data+12, 20);
	pcontent += 20;

	char reservered_two[7] = {0};
	memcpy(pcontent, &reservered_two, sizeof(reservered_two));
	pcontent += 7;

	uint16 peer_num = 3;
	memcpy(pcontent, &peer_num, sizeof(peer_num));
	pcontent += 2;
	
	memcpy(pcontent, peer_reserved, sizeof(peer_reserved));
	pcontent += 3;
	
	memcpy(pcontent, peer_ptr, peer_data_len);
	pcontent += peer_data_len;

	char unkown[] = {(char)0x0b, (char)0x0c, (char)0xa8, (char)0x00, (char)0xba, (char)0xc2, (char)0x52, 
						(char)0x00, (char)0xca, (char)0x00, (char)0x32, (char)0x1b};
	memcpy(pcontent, &unkown, sizeof(unkown));
	pcontent += sizeof(unkown);

	char reserved_three[] = {(char)0x1b, (char)0x1b, (char)0x1b, (char)0x1d};
	memcpy(pcontent, reserved_three, sizeof(reserved_three));
	pcontent += 4;

	char reserved_four[] = {(char)0x00, (char)0x00, (char)0x00, (char)0x00};
	memcpy(pcontent, reserved_four, sizeof(reserved_four));
	pcontent += 4;

	char reserved_five[] = {(char)0x46, (char)0x14, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 
									, 0x00 , 0x00 , 0x00 , 0x00};
	memcpy(pcontent, reserved_five, sizeof(reserved_five));
	pcontent += sizeof(reserved_five);

	char reserved_six[] = {(char)0x6e, (char)0x21, (char)0xd6, (char)0x41, (char)0x4d, (char)0x45, (char)0xe6, 
								(char)0x40, (char)0x6b, (char)0xe4, (char)0x3f, (char)0x40, (char)0x3a, (char)0xdb, (char)0xff};
	memcpy(pcontent, reserved_six, sizeof(reserved_six));
	pcontent += sizeof(reserved_six);

	char reserved_seven[] = {(char)0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	memcpy(pcontent, reserved_seven, sizeof(reserved_seven));
	pcontent += sizeof(reserved_seven);

	char reserved_eight[] = {(char)0x00, 0x00, (char)0xe6, 0x00, 0x00, 0x00, (char)0x6d, 0x1e, 0x00, 0x00, (char)0x14, 0x0a, 0x00, 0x00, (char)0x7c, 0x0c};
	memcpy(pcontent, reserved_eight, sizeof(reserved_eight));
	pcontent += sizeof(reserved_eight);

	char reserved_nine[] = {(char)0x00, 0x00, (char)0xdd, (char)0x07};
	memcpy(pcontent, reserved_nine, sizeof(reserved_nine));
	pcontent += sizeof(reserved_nine);
	
	char reserved_ten[6] = {0};
	memcpy(pcontent, reserved_ten, sizeof(reserved_ten));
	pcontent += sizeof(reserved_ten);

	char reserved_eleven[] = {(char)0xfe, (char)0x78, (char)0xff};
	memcpy(pcontent, reserved_eleven, sizeof(reserved_eleven));
	pcontent += sizeof(reserved_eleven);

	uint16 packet_len = pcontent - buf;
	memcpy(buf, &packet_len, sizeof(packet_len));

	len = packet_len;

}


/*------------------------------------------------------------------------------
 * ��  ��: PpsHandshakeProcessor���캯��
 * ��  ��: [in] session PpsSession����ָ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.27
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
PpsHandshakeProcessor::PpsHandshakeProcessor(PpsSessionSP session)
	: pps_session_(session) 
{
}

/*------------------------------------------------------------------------------
 * ��  ��: ����pps��handshake����
 * ��  ��: [in] data ���ݰ�������
 *         [in] length ���ݰ��ĳ���
 * ����ֵ: �����ݰ��Ƿ��Ѿ�����
 * ��  ��:
 *   ʱ�� 2013.12.27
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsHandshakeProcessor::Process(const void* data, uint32 length)
{
	//�������PPS���ģ��������������Ҷϴ���
	return true;
}

}

