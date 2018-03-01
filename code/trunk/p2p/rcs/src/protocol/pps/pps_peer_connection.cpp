/*#############################################################################
 * �ļ���   : pps_peer_connection.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��30��
 * �ļ����� : PpsPeerConnection��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_peer_connection.hpp"

#include "bc_assert.hpp"
#include "bc_assert.hpp"
#include "bc_io.hpp"
#include "pps_fsm.hpp"
#include "pps_metadata_retriver.hpp"
#include "pps_msg_recognizer.hpp"
#include "pps_pub.hpp"
#include "pps_session.hpp"
#include "pps_torrent.hpp"
#include "disk_io_job.hpp"
#include "policy.hpp"
#include "rcs_config.hpp"
#include "session.hpp"
#include "socket_connection.hpp"
#include "rcs_util.hpp"
#include "net_byte_order.hpp"

namespace BroadCache
{
static const uint32 kPpsMetadataPieceSize = 1 * 1024; // Э��涨
static const uint32 kPpsBlockSize = 1 * 1024;

const int kExchangeCode[2] = { 0x80, 0x81 };

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯�� 
 * ��  ��: ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��12��30��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsPeerConnection::PpsPeerConnection(Session& session, const SocketConnectionSP& sock_conn
	, PeerType peer_type, uint64 download_bandwidth_limit, uint64 upload_bandwidth_limit)
	: PeerConnection(session, sock_conn, peer_type, download_bandwidth_limit, upload_bandwidth_limit)
	, support_metadata_extend_(false)
	, retrive_metadata_failed_(false)
	, handshake_complete_(false)
{

}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.27
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsPeerConnection::~PpsPeerConnection()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��״̬��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��24��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::Initialize()
{
	SetFragmentSize(PPS_FRAGMENT_SIZE);  //����ppsfragment��С

	FsmStateSP state;
	
	state.reset(new PpsFsmInitState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_INIT, state);

	state.reset(new PpsFsmHandshakeState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_HANDSHAKE, state);

	state.reset(new PpsFsmMetadataState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_METADATA, state);

	state.reset(new PpsFsmBlockState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_BLOCK, state);

	state.reset(new PpsFsmTransferState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_TRANSFER, state);

	state.reset(new PpsFsmUploadState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_UPLOAD, state);

	state.reset(new PpsFsmDownloadState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_DOWNLOAD, state);

	state.reset(new PpsFsmSeedState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_SEED, state);

	state.reset(new PpsFsmCloseState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_CLOSE, state);
	
}

/*-----------------------------------------------------------------------------
 * ��  ��: 
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.27
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::SecondTick()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����PPSЭ�����Ϣʶ����
 * ��  ��:
 * ����ֵ: ��Ϣʶ�����б�
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
std::vector<MsgRecognizerSP> PpsPeerConnection::CreateMsgRecognizers()
{
	std::vector<MsgRecognizerSP> recognizers;

	recognizers.push_back(PpsCommonMsgRecognizerSP(new PpsCommonMsgRecognizer()));

	return recognizers;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���keep-alive��Ϣ
 * ��  ��: [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ�ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteKeepAliveMsg(char* buf, uint64& len)
{

}

/*-----------------------------------------------------------------------------
 * ��  ��: ��metadata���� ���� metadata������Ϣ
 * ��  ��: [in] piece_index ��Ϣ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::SendMetadataRequestMsg(uint64 piece_index, uint64 data_len)
{
	LOG(INFO) << "Send PPS metadata request msg | piece: " << piece_index;

	SendBuffer send_buf = session().mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	WriteMetadataRequestMsg(piece_index, data_len, send_buf.buf, send_buf.len);

	socket_connection()->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����metadata������Ӧ��Ϣ
 * ��  ��: [in] piece_index ��Ϣ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��23��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::SendMetadataDataMsg(uint64 piece_index)
{
	LOG(INFO) << "Send PPS metadata data msg | piece: " << piece_index;

	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	SendBuffer send_buf = session().mem_buf_pool().AllocMsgBuffer(MSG_BUF_DATA);

	WriteMetadataDataMsg(piece_index, send_buf.buf, send_buf.len);

	socket_connection()->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Metadata������Ϣ
 * ��  ��: [in] piece_index ��Ϣ����
 *         [out] buf  ������
 *         [out] len  ��������С
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteMetadataRequestMsg(int piece_index, uint64 data_len, 
																char* buf, uint64& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	LOG(INFO) << "Write Pps metadata request msg";

	char* buffer = buf;

    buffer = buffer + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer);	//��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_REQUEST, buffer);

	IO::write_uint32(seq_id_, buffer);

	std::memcpy(buffer, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
	buffer += 20;

	IO::write_uint32(0xffff0000, buffer);  //��Ӧ��metadata����,��������0xffff0000

	IO::write_uint32_be(piece_index * kPpsBlockSize, buffer);  //block offset, kPpsBlockSize Ӧ�ô�torrent��ȡ����Ҫ�Ľ�

	IO::write_uint32_be((uint32)data_len, buffer);  // request data len

	IO::write_uint8(0xff, buffer);

	std::memset(buffer, 0, 9);
	buffer += 9;

	IO::write_uint32_be(0x10, buffer); 

	std::memcpy(buffer, PpsSession::GetLocalPeerId(), 16); //���16λ��peer id
    buffer += 16;

	IO::write_uint32_be(0x01, buffer);
	IO::write_uint32_be(0x01, buffer);

	uint16 packet_len = buffer - buf -4; //metadata���ü�4 
	char* front_pos = buf;
	IO::write_uint16_be(packet_len, front_pos); 

	len = packet_len + 4;

    return ;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Metadata������Ӧ��Ϣ
 * ��  ��: [in] piece_index ��Ϣ����
 *         [out] buf  ������
 *         [out] len  ��������С
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��23��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteMetadataDataMsg(int piece_index, char* buf, uint64& len)
{
	len = 0; // ��ʼ��һ��

	PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, torrent().lock());
	if (!pps_torrent) return;

	uint32 metadata_size = pps_torrent->GetMetadata().size();
	if (metadata_size == 0)
	{
		LOG(ERROR) << "No metadata avaliable";
		return;
	}

	if (static_cast<uint32>(piece_index) > metadata_size)
	{
		LOG(ERROR) << "Invalid piece index | " << piece_index;
		return;
	}

	LOG(INFO) << "Write Pps metadata data response msg";

	char* buffer = buf;

	buffer = buffer + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_PIECE, buffer);

	char* checksum_one_ptr = buffer;
	buffer += 2;
	char* checksum_two_ptr = buffer;
	buffer += 2;
	
	unsigned char* checksum_one_offset =(unsigned char*)buffer;  // checksum_one ȡƫ����11��ʼȡ44���ֽڼ���
	
	IO::write_uint32(0xFF, buffer);
	IO::write_uint32(seq_id_, buffer);

	std::memcpy(buffer, pps_torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
	buffer += 20;

	IO::write_uint32(0xFFFF, buffer);  //PeerRequest.piece ��64�ֽڣ� �˴���32�ֽ�

	IO::write_uint32(piece_index, buffer);  //piece offset

	IO::write_uint32(kPpsBlockSize, buffer);  // request data len

	const char* metadata_start = pps_torrent->GetMetadata().c_str() + piece_index;

	uint32 metadata_len = kPpsBlockSize;
	if (static_cast<uint32>(piece_index) == metadata_size / kPpsBlockSize)
	{
		metadata_len = metadata_size % kPpsBlockSize;
	}

	std::memcpy(buf, metadata_start, metadata_len);
	buf += metadata_len;


	std::memset(buffer, 0, 24);
	buffer += 24;

	IO::write_uint32(0, buffer);
	IO::write_uint32(0x01, buffer);

	std::memset(buffer, 0, 12);
	buffer  += 12;

	// ʣ��9�ֽڵ�δ֪�ֽ�,������Ҫ�ƽ�
	char unkown[] = {(char)0x59, (char)0x6f, (char)0x18, (char)0x55, (char)0x00, 
						(char)0x0a, (char)0x00, (char)0xb6, (char)0x00};
	std::memcpy(buffer, unkown, sizeof(unkown));
	buffer += sizeof(unkown);

	//checksum_two ȡ������16�ֽڽ��м���
	unsigned char* checksum_two_offset =(unsigned char*)(buffer - 16);   
	
	//���� checksum, ���в�������Ϊ0x18
	uint16 checksum_one = CalcChecksum(checksum_one_offset, 44, 0x18);
	IO::write_uint16(checksum_one, checksum_one_ptr);
	uint16 checksum_two = CalcChecksum(checksum_two_offset, 16, 0x18);
	IO::write_uint16(checksum_two, checksum_two_ptr);

	uint16 packet_len = buffer - buf; //data response �����Ȳ��ü�4
	char* front_pos = buf;
	IO::write_uint16_be(packet_len, front_pos); 

	len = packet_len;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������������Ϣ
 * ��  ��: [in] request ���������
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteDataRequestMsg(const PeerRequest& request, char* buf, uint64& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	LOG(INFO) << "Write Pps data request msg";

	char* buffer = buf;

	buffer = buffer + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_REQUEST, buffer);

	IO::write_uint32(seq_id_, buffer);

	std::memcpy(buffer, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
	buffer += 20;

	IO::write_uint32_be(request.piece, buffer);  //PeerRequest.piece ��64�ֽڣ� �˴���32�ֽ�

	IO::write_uint32_be(request.start,buffer);  //piece offset

	IO::write_uint32_be(request.length, buffer);  // request data len

	IO::write_uint8(0xff, buffer);

	std::memset(buffer, 0, 9);
	buffer += 9;

	IO::write_uint32_be(0x10, buffer); 

	std::memcpy(buffer, PpsSession::GetLocalPeerId(), 16); //���16λ��peer id
	buffer += 16;

	IO::write_uint32_be(0x01, buffer);
	IO::write_uint32_be(0x01, buffer);

	uint16 packet_len = buffer - buf; //metadata���ü�4 
	char* front_pos = buf;
	IO::write_uint16_be(packet_len, front_pos); 

	len = packet_len;

	return ;

}

/*-----------------------------------------------------------------------------
 * ��  ��: �����չ����������Ϣ
 * ��  ��: [in] request ���������
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteDataExtendedRequestMsg(const PeerRequest& request, 
															char* buf, uint64& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	LOG(INFO) << "Write Pps extended data request msg";

	char* buffer = buf;

	buffer = buffer + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_EXTEND_REQUEST, buffer);

	IO::write_uint32(seq_id_, buffer);

	IO::write_uint16(0x0d, buffer);  // unkown

	IO::write_uint32(request.start, buffer); //offset

	IO::write_uint16(request.length, buffer);  // PeerRequest.length ��64�ֽڣ� �˴���16�ֽ�

	char unkown[] = {(char)0x00, (char)0x08, (char)0x00, (char)0x00};
	std::memcpy(buffer, unkown, sizeof(unkown));
	buffer += sizeof(unkown);

	IO::write_uint16(0x06, buffer);

	uint16 packet_len = buffer - buf; //metadata���ü�4 
	IO::write_uint16(packet_len, buf);

	len = packet_len;

	return ;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ݻظ���Ϣ
 * ��  ��: [in] job block����
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteDataResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	LOG(INFO) << "Write Pps data response msg";

	char* buffer = buf;

	buffer = buffer + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_PIECE, buffer);

	char* checksum_one_ptr = buffer;
	buffer += 2;
	char* checksum_two_ptr = buffer;
	buffer += 2;
	
	unsigned char* checksum_one_offset =(unsigned char*)buffer;  // checksum_one ȡƫ����11��ʼȡ44���ֽڼ���
	
	IO::write_uint32(0xFF, buffer);
	IO::write_uint32(seq_id_, buffer);

	std::memcpy(buffer, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
	buffer += 20;

	IO::write_uint32(job->piece, buffer);  //PeerRequest.piece ��64�ֽڣ� �˴���32�ֽ�

	IO::write_uint32(job->offset, buffer);  //piece offset

	IO::write_uint32(job->len, buffer);  // request data len

	std::memcpy(buffer, job->buf, job->len); //block data
	buffer += job->len;

	std::memset(buffer, 0, 24);
	buffer += 24;

	// ʣ��9�ֽڵ�δ֪�ֽ�,������Ҫ�ƽ�
	char unkown[] = {(char)0x59, (char)0x6f, (char)0x18, (char)0x55, (char)0x00, 
							(char)0x0a, (char)0x00, (char)0xb6, (char)0x00};
	std::memcpy(buffer, unkown, sizeof(unkown));
	buffer += sizeof(unkown);

	//checksum_two ȡ������16�ֽڽ��м���
	unsigned char* checksum_two_offset =(unsigned char*)(buffer - 16);   
	
	//���� checksum, ���в�������Ϊ0x18
	uint16 checksum_one = CalcChecksum(checksum_one_offset, 44, 0x18);
	IO::write_uint16(checksum_one, checksum_one_ptr);
	uint16 checksum_two = CalcChecksum(checksum_two_offset, 16, 0x18);
	IO::write_uint16(checksum_two, checksum_two_ptr);

	uint16 packet_len = buffer - buf; //data response �����Ȳ��ü�4
	char* front_pos = buf;
	IO::write_uint16_be(packet_len, front_pos); 

	len = packet_len;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ݻظ���Ϣ
 * ��  ��: [in] job block����
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteDataExtendedResponseMsg(const ReadDataJobSP& job, 
																	char* buf, uint64& len)
{
	//��ǰ�ϰ汾û�д�������ֶ�,��������ͨ�ţ��˰汾Ҳ����ʵ�ִ�Э��

	LOG(INFO) << "Write Pps extend data response msg";

	char* buffer = buf;

	buffer = buffer + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buffer); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_EXTEND_PIECE, buffer);

	IO::write_uint8(0x01, buffer);

	uint32 checksum = 0xaf012843;
	IO::write_uint32(checksum, buffer); 

	IO::write_uint32(0xFFFFFFFF, buffer);

	IO::write_uint32(seq_id_, buffer);

	IO::write_uint32(0x06, buffer); //unkown

	IO::write_uint32(job->offset, buffer);  //piece offset

	IO::write_uint32(job->len, buffer);  // request data len

	std::memcpy(buffer, job->buf, job->len); //block data
	buffer += job->len;

	IO::write_uint16(0, buffer);

	// ʣ��5�ֽڵ�δ֪�ֽ�,������Ҫ�ƽ�
	char unkown[] = {(char)0x91, (char)0xcd, (char)0x28, (char)0x76, (char)0x00};
	std::memcpy(buffer, unkown, sizeof(unkown));
	
	uint16 packet_len = buffer - buf; //metadata���ü�4 
	IO::write_uint16(packet_len, buf);

	len = packet_len;
    
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���hava��Ϣ, ����PPSʵ�����Ƿ���bitfiled����
 * ��  ��: [in] piece_index piece index
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WriteHaveMsg(int piece_index, char* buf, uint64& len)
{
	ConstructBitfieldMsg(true, buf, len);	
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����������ݻ�ȡ��Ϣ����
 * ��  ��: [in] buf ��Ϣ����
 *         [in] uint64 ��Ϣ���ݵĳ���
 * ����ֵ: ��Ϣ����
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsMsgType PpsPeerConnection::GetMsgType(const char* buf, uint64 len)
{
	//��ȡ����֮���3�ֽڣ�ƥ���ʶ�ֶ�
	buf = buf + 3;
	PpsMsgType pkt_type = static_cast<PpsMsgType>(IO::read_uint32(buf));
	//PpsMsgType msg_type = SwitchToMsgType(pkt_type);

	LOG(INFO) << "Pps msg type: " << pkt_type;
	
	return pkt_type;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����metadata������Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 		   [in] msg_data �������ص���Ϣ
 * ����ֵ: �����ɹ����
 * ��  ��:
 *   ʱ�� 2013��11��23��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParseMetadataRequestMsg(const PpsRequestMsg& request_msg,
	                                           PpsMetadataRequestMsg& msg_data)
{
	msg_data.piece = request_msg.begin / kPpsMetadataPieceSize;
	msg_data.len = request_msg.length;

	return true;	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���� metadata ��Ϣ��data����
 *         ��Ϣ��ʽ��
 *         
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 		   [in] msg_data �������ص���Ϣ��PpsMetadataMsg
 * ����ֵ: �����ɹ����
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParseMetadataDataMsg(const PpsPieceMsg& piece_msg,
	                                        PpsMetadataDataMsg& msg_data)
{
	msg_data.piece = piece_msg.begin / kPpsMetadataPieceSize;
	msg_data.len = piece_msg.data_len;
	msg_data.buf = piece_msg.block_data->c_str();

	return true;	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����have��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParsePpsHaveMsg(const char* buf, uint64 len, PpsHaveMsg& msg)
{
    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����bitfield��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParsePpsBitfieldMsg(const char* buf, uint64 len, PpsBitfieldMsg& msg)
{
	const char* buffer = buf;
	
	buf += 2; //packet_len
	buf += 5; //type
	buf += 2; //checksum

	msg.passive = IO::read_uint8(buf);

	buf += 4; //unkown
	buf += 20;

	msg.bitfield.clear();

	if (!msg.passive)
	{
		buf += 1;
	}
	
    msg.bitfield.clear();
    const char* bit_data = buf; //bitfield���ݿ�ʼλ��
    uint32 bitfield_len = len - (buf - buffer) ; //bitfield���ݳ���
    
    uint32 i = 0;
    while (i < bitfield_len)
    {
        char ch = bit_data[i]; //��ȡ��һλ��Ӧ���ֽ�
        for (int j = 7; j >= 0; --j)
        {
            msg.bitfield.push_back((ch & (1 << j)) != 0);
        }
        ++i;
    }

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����request��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParsePpsRequestMsg(const char* buf, uint64 len, PpsRequestMsg& msg)
{
	uint16 msg_length = IO::read_uint16(buf); //Э���ַ�������

	BC_ASSERT(msg_length + 4 >= (uint16)len);

	buf += 2; //packet_len
	buf += 5;  //protocol no
	buf += 4; //seq_id
	buf += 20; //hash

	msg.index = IO::read_uint32(buf);
	msg.begin = IO::read_uint32(buf);
	msg.length = IO::read_uint32(buf);
	
	//Todo ʣ��ı���������ʱ��������
	
    return true;
} 

/*-----------------------------------------------------------------------------
 * ��  ��: ����extended request��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2014��1��5��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParsePpsExtendedRequestMsg(const char* buf, uint64 len, 
																PpsRequestMsg& msg)
{
	uint16 msg_length = NetToHost<uint16>(buf); //Э���ַ�������
	BC_ASSERT(msg_length + 4 >= (uint16)len);

	buf += 2; //packet_len
	buf += 5;  //protocol no
	buf += 4; //seq_id
	buf += 2; //unkown
	
	msg.index = 1; // msg.begin / piece_length;
	
	msg.begin = IO::read_uint32(buf);
	msg.length = IO::read_uint16(buf);

	//ʣ��6�ֽ��ݲ�������
	
    return true;
} 


/*-----------------------------------------------------------------------------
 * ��  ��: ����Pps��piece��Ϣ�������Pps�����������Ӧ��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2014.1.2
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParsePpsPieceMsg(const char* buf, uint64 len, PpsPieceMsg& msg)
{  
	uint16 msg_length = NetToHost<uint16>(buf); //Э���ַ�������
	BC_ASSERT(msg_length + 4 >= (uint16)len);

	buf += 2;
	buf += 5;  //protocol no

	msg.checksum_front = IO::read_int16(buf);
	msg.checksum_end = IO::read_int16(buf);

	buf += 4; // 0xFF

	msg.seq_id = IO::read_uint32_be(buf);

	buf += 20; //info_hash

	msg.index = IO::read_uint32_be(buf);

	msg.begin = IO::read_uint32_be(buf);

	msg.data_len = IO::read_uint32_be(buf);

    msg.block_data.reset(new std::string(buf, msg.data_len));

	//Todo ʣ��ı���������ʱ��������
	
    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����handshake��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.12.30
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsPeerConnection::ParsePpsHandshakeMsg(const char* buf, uint64 len, PpsHandshakeMsg& msg)
{
    uint16 protocol_length = NetToHost<uint16>(buf); //Э���ַ�������
	msg.length = protocol_length;
	const char* origin_pos = buf;
	buf += 2;

	std::memcpy(&msg.protocol_id, buf, 5);
	buf += 5;
	
	std::memcpy(&msg.info_hash, buf, 20);
	buf += 20;
	
	buf += 2;  // 0x0002
	buf += 8;

	std::memcpy(&msg.network_type, buf, 1);
	buf += 1;

	buf += 8; // 0x00000000  + 0x80 + ## + 0x00

	std::string temp_str(buf, len - (buf - origin_pos));
	uint16 bitfiled_end_flag = 0x0028;
	size_t pos = temp_str.find(bitfiled_end_flag);
	if ( pos == std::string::npos) return false;
	
	msg.bitfield.clear();
	
	const char* bit_data = buf; //bitfield���ݿ�ʼλ��
	uint32 bitfield_len = pos - 1; //bitfield���ݳ���
		
	uint32 i = 0;
	while (i < bitfield_len)
	{
		char ch = bit_data[i]; //��ȡ��һλ��Ӧ���ֽ�
		for (int j = 7; j >= 0; --j)
		{
			msg.bitfield.push_back((ch & (1 << j)) != 0);
		}
		++i;
	}

	pos = temp_str.find(std::string(msg.info_hash, 20));
	if (pos == std::string::npos) return false;
	
	buf = buf + pos + 20;  // info hash
	buf += 4; 

	std::memcpy(&msg.peer_id, buf, 16);
	buf += 16;

	buf += 4; //0x00

	//ʣ���ֽ��ݲ�������
	
    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����peer_id����peertype
 * ��  ��: [in]  peer_id
 * ����ֵ: ����peerclient������ PeerclientType
 * ��  ��:
 *   ʱ�� 2013.12.29
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PeerClientType PpsPeerConnection::GetPeerClientType( const std::string& peer_id)
{
	const char * ClentsTypeSymbol[] = {"", "BC", "XL", "BX", "UT", "SP" ,"SD"};
	std::string peer_client_type_symbol = peer_id.substr(1,2);  //����peer_id �ַ����ڶ�λ������λ�ַ�
	PeerClientType peer_client = PEER_CLIENT_UNKNOWN;

	for (uint32 i = 0; i < sizeof(ClentsTypeSymbol)/sizeof(ClentsTypeSymbol[0]); ++i)
	{
		if (std::strcmp(peer_client_type_symbol.c_str(),ClentsTypeSymbol[i]))
		{
			if(6 == i) 
			{
				peer_client = PEER_CLIENT_XUNLEI;
				break;
			}
			peer_client = static_cast<PeerClientType>(i);
			break;	
		}
	}
	return peer_client;	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���piece_map
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.12.30
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::SetPieceMap(char* buf, uint32& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	PiecePicker* piece_picker = torrent->piece_picker();
	if (!piece_picker) return;

//	LOG(INFO) << "Display Pps bitfield msg | " 
//		      << GetReversedBitset(piece_picker->GetPieceMap());

	const boost::dynamic_bitset<>& piece_map = piece_picker->GetPieceMap();

    size_t bits = (7 + piece_map.size()) / 8;  // λ���ֽ���
    std::memset(buf, 0, bits);  // ����������

    for (size_t i = 0; i < piece_map.size(); ++i)
    {
        if (piece_map[i])
        {
            buf[i / 8] |= (1 << (7 - i % 8));  // ���ö�Ӧλ
        }
    }

	len = bits;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����peer_id����peertype
 * ��  ��: ���bitfield��Ϣ
 * ��  ��: [in] piece-map bitfield��Ϣ
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len bitfield��Ϣ�ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsPeerConnection::WritePieceMapMsg(const boost::dynamic_bitset<>& piece_map, char* buf, 
													uint64& len, bool passive)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	char* buffer = buf;
	buf += 2;

	IO::write_uint8(kProtoPps, buf); //��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_BITFIELD, buf);

	char* checksum_pos = buf;
	buf += 2;
	uint8* checksum_offset = (uint8*)buf;  //ȡ��10���ֽڵ�����β���� checksum

	IO::write_uint16(passive, buf);

	if (passive)
	{
		IO::write_uint32(0x0, buf);
	}
	else
	{
		IO::write_uint32(0xffffffff, buf);
	}

	std::memcpy(buf, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
    buf += 20;

	if (!passive)
	{
		IO::write_uint8(piece_map.size(), buf);
	}

	size_t bits = (7 + piece_map.size()) / 8;  // λ���ֽ���
    std::memset(buf, 0, bits);  // ����������
    for (size_t i = 0; i < piece_map.size(); ++i)
    {
        if(piece_map[i])
        {
            buf[i / 8] |= (1 << (7 - i % 8));  // ���ö�Ӧλ
        }
    }
	buf += bits;

	uint16 packet_len = buf - buffer; //bitfiled���Ĳ��ü�4
	IO::write_uint16(packet_len, buffer);

	//����checksum
	uint16 checksum = CalcChecksum(checksum_offset, packet_len - 9, 0x18);
	IO::write_int16(checksum, checksum_pos);
	
    len = packet_len; //���÷������ݳ���
	
}

bool PpsPeerConnection::OnChokeMsg(MsgId msg_id, const char* data, uint64 length)
{
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�have��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪPPS_MSG_HAVE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnHaveMsg(MsgId msg_id, const char* data, uint64 length)
{
    PpsHaveMsg msg;
    ParsePpsHaveMsg(data, length, msg);

	LOG(INFO) << "Received PPS have piece msg | " << msg.piece_index;

    ReceivedHaveMsg(msg.piece_index);

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�bitfield��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪPPS_MSG_BITFIELD
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnBitfieldMsg(MsgId msg_id, const char* data, uint64 length)
{
    PpsBitfieldMsg msg;
    ParsePpsBitfieldMsg(data, length, msg); 
    ReceivedBitfieldMsg(msg.bitfield);

    LOG(INFO) << "Received PPS bitfield msg : " << GetReversedBitset(msg.bitfield);

	if (!msg.passive)
	{
		SendBitfieldMsg(!msg.passive);
	}

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�request��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪPPS_MSG_REQUEST
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnExtendedRequestMsg(MsgId msg_id, const char* data, uint64 length)
{
    PpsRequestMsg msg; 
    ParsePpsExtendedRequestMsg(data, length, msg);

    LOG(INFO) << "Received PPS extended request msg | " << "[" << msg.index 
	      << " : " << msg.begin << " : " << msg.length << "]";

	//metadata �� data �õ���ͬһ��Э�飬���ڴ˴������ǽ��зֱ��� 
	ReceivedDataRequest(PeerRequest(msg.index, msg.begin, msg.length));
	
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�request��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪPPS_MSG_REQUEST
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnRequestMsg(MsgId msg_id, const char* data, uint64 length)
{
    PpsRequestMsg msg; 
    ParsePpsRequestMsg(data, length, msg);

    LOG(INFO) << "Received PPS request msg | " << "[" << msg.index 
		      << " : " << msg.begin << " : " << msg.length << "]";

	//metadata �� data �õ���ͬһ��Э�飬���ڴ˴������ǽ��зֱ��� 
	if (msg.index != 0xffff0000)
	{
		ReceivedDataRequest(PeerRequest(msg.index, msg.begin, msg.length));
	}
	else
	{
		OnMetadataRequestMsg(msg);
	}
	
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�piece��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪPPS_MSG_PIECE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnPieceMsg(MsgId msg_id, const char* data, uint64 length)
{
    PpsPieceMsg msg;
    ParsePpsPieceMsg(data, length, msg);

	if (msg.data_len == 0) return false;

	LOG(INFO) << "Received PPS piece msg | " << "[" << msg.index 
		<< ":" << msg.begin << ":" << msg.data_len << "]";

 	//������ͨpiece��Ӧ����metadata��
	if (msg.index != 0x0000ffff)
	{
		block_data_ = msg.block_data;
	    ReceivedDataResponse(PeerRequest(msg.index, msg.begin, 
	        msg.block_data->size()), msg.block_data->c_str());
	}
	else
	{
		OnMetadataDataMsg(msg);
	}

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�keep-alive��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪPPS_MSG_KEEPALIVE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnKeepAliveMsg(MsgId msg_id, const char* data, uint64 length)
{
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�handshake��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪPPS_MSG_HANDSHAKE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnHandshakeMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received Pps handshake msg";

    PpsHandshakeMsg msg;
    ParsePpsHandshakeMsg(data, length, msg);

	peer_id_.assign(msg.peer_id, 16);
	
	ReceivedBitfieldMsg(msg.bitfield);  //����bitfiled

    const char* info_hash = msg.info_hash;
    ObtainedInfoHash(InfoHashSP(new PpsInfoHash(std::string(info_hash, info_hash + 20))));

	// �յ��Զ˵�HandShake��Ϣ�����Ϸ���һ��HandShake��Ȼ��֪ͨ�ϲ��������
	if (socket_connection()->connection_type() == CONN_PASSIVE)
    {
        SendHandshakeMsg(true);
	}
    else if (socket_connection()->connection_type() == CONN_ACTIVE)
	{
		// ����metadata��ȡ��־ �����bt��pps��udp�Ĳ�ȷ���� �ڴ˴����ñȽϺ���
		support_metadata_extend_ = true;
	}

	handshake_complete_ = true;

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: metadata������Ӧ��Ϣ����
 * ��  ��: [in] msg_id ��Ϣ��
 *         [in] data ��Ϣ����
 *         [in] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsPeerConnection::OnMetadataDataMsg(const PpsPieceMsg& piece_msg)
{
    LOG(INFO) << "Received metadata data msg";

	PpsMetadataDataMsg msg;
    if (!ParseMetadataDataMsg(piece_msg, msg))  //�м��һ����������
	{
		LOG(ERROR) << "Fail to parse metadata data message";
		return false;
	}

	PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, torrent().lock());
	if (!pps_torrent) return false;
	
	// ��������Ӧ���Ľ���MetadataRetriver����
	pps_torrent->metadata_retriver()->ReceivedMetadataDataMsg(msg);

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����metadata������Ϣ����
 * ��  ��: [in] msg_id ��Ϣ��
 *         [in] data ��Ϣ����
 *         [in] length ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsPeerConnection::OnMetadataRequestMsg(const PpsRequestMsg& request_msg)
{
	LOG(INFO) << "Received metadata request message";
	
	PpsMetadataRequestMsg msg;
	if (!ParseMetadataRequestMsg(request_msg, msg))
	{
		LOG(ERROR) << "Fail to parse metadata request msg";
		return false;
	}

	SendMetadataDataMsg(msg.piece);

	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���յ�����ʶ���bt��Ϣ��Ĵ�����
 * ��  ��: [in] msg_id ��Ϣ��
 *         [in] data ��Ϣ��Ӧ������ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsPeerConnection::OnUnknownMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received unknown msg | " << msg_id;

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����handshake��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.31
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsPeerConnection::SendHandshakeMsg(bool reply)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	LOG(INFO) << "Send Pps handshake response msg";

	MemBufferPool& mem_pool = torrent->session().mem_buf_pool();
	SendBuffer send_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

    char* buf = send_buf.buf;

	buf = buf + 2;  //�Ȳ�����packet_len

	IO::write_uint8(kProtoPps, buf);	//��ʶ�ֶ�
	IO::write_uint32(PPS_MSG_HANDSHAKE, buf);

	std::memcpy(buf, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
    buf += 20;

	IO::write_uint16(0x0002, buf);

	const char kVersion[] = {(char)0x03, (char)0x00, (char)0x00, (char)0x00, (char)0x0B, (char)0x00, 
							 (char)0x82, (char)0x01};
	std::memcpy(buf, kVersion, 8);
	buf += sizeof(kVersion);
	
	IO::write_uint8(0x0b, buf); ////0D-->0B because of outer network

	IO::write_uint32(0x0, buf); //00->01  01 not ok

	IO::write_uint8(kExchangeCode[reply], buf);

	IO::write_uint8('#', buf);
  	IO::write_uint8('#', buf);
	IO::write_uint8(0x0, buf);

	// ����bitfield
	uint32 bitfield_len = 0;
	SetPieceMap(buf, bitfield_len);
	buf += bitfield_len;

	IO::write_uint8(0x0, buf);
	IO::write_uint8(0x28, buf);
	IO::write_uint32(0x0, buf);
	IO::write_uint32(0x0, buf);
	IO::write_uint8(0x01, buf);

	std::memcpy(buf, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
    buf += 20;

	IO::write_uint32_be(0x10, buf);

	std::memcpy(buf, PpsSession::GetLocalPeerId(), 16); //���16λ��peer id
    buf += 16;

	if (reply)
	{
		IO::write_uint32(0x00, buf); //�˴�Ӧ��checksum? ��ȷ���ֶ�
	}

	IO::write_uint32(0x00, buf);

	if (reply)
	{
		const char unkown_one[] = {(char)0xb5, (char)0xe7, (char)0xd0, (char)0xc5, (char)0x00, 
									(char)0xd6, (char)0xd0, (char)0xb9, (char)0xfa};
		std::memcpy(buf, unkown_one, sizeof(unkown_one));
		buf += sizeof(unkown_one);
	}
	else
	{
		//��������ʱ�˴���15�ֽ�
		const char unkown_one[] = {(char)0x31, (char)0x00, (char)0x32, (char)0x00, (char)0x33, 
									(char)0x00, (char)0xb5, (char)0xe7, (char)0xd0, (char)0xc5, 
									(char)0x00, (char)0xd6, (char)0xd0, (char)0xb9, (char)0xfa};
		std::memcpy(buf, unkown_one, sizeof(unkown_one));
		buf += sizeof(unkown_one);
	}

	IO::write_uint8(0x00, buf);

	const char unkown_two[] = {(char)0xbb, (char)0xaa, (char)0xc4, (char)0xcf, (char)0x00, (char)0xb9, 
							   (char)0xe3, (char)0xb6, (char)0xab, (char)0x00, (char)0xc9 , (char)0xee , 
							   (char)0xdb , (char)0xda, (char)0x00};
	std::memcpy(buf, unkown_two, sizeof(unkown_two));
	buf += sizeof(unkown_two);

	uint16 packet_len = buf - send_buf.buf -4; // handshake�ı��Ķμ���Ҫ��4
	char* front_pos = send_buf.buf;
	IO::write_uint16_be(packet_len, front_pos); //����ֱ����send_buf.buf,writeϵ�к������buffer��λ

    send_buf.len = packet_len + 4; //���÷������ݳ���

    socket_connection()->SendData(send_buf); //����handshake��Ϣ
}

/*------------------------------------------------------------------------------
 * ��  ��: ����bitfield��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsPeerConnection::ConstructBitfieldMsg(bool passive, char* buf, uint64& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

    PiecePicker* piece_picker = torrent->piece_picker();
	if (!piece_picker) return;

	LOG(INFO) << "Send PPS bitfield msg | " 
		      << GetReversedBitset(piece_picker->GetPieceMap());

	WritePieceMapMsg(piece_picker->GetPieceMap(), buf, len, passive);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����bitfield��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsPeerConnection::SendBitfieldMsg(bool passive)
{

	MemBufferPool& mem_pool = session().mem_buf_pool();

	SendBuffer send_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

	ConstructBitfieldMsg(passive, send_buf.buf, send_buf.len);

	socket_connection()->SendData(send_buf);  
}

/*------------------------------------------------------------------------------
 * ��  ��: ����keep-alive��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsPeerConnection::SendKeepAliveMsg()
{

}

}
