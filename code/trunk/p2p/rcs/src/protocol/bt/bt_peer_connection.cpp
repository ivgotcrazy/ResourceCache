/*#############################################################################
 * �ļ���   : bt_peer_connection.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��13��
 * �ļ����� : BtPeerConnection��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bt_peer_connection.hpp"

#include "bc_assert.hpp"
#include "bc_assert.hpp"
#include "bc_io.hpp"
#include "bt_fsm.hpp"
#include "bt_metadata_retriver.hpp"
#include "bt_msg_recognizer.hpp"
#include "bt_net_serializer.hpp"
#include "bt_pub.hpp"
#include "bt_session.hpp"
#include "bt_torrent.hpp"
#include "disk_io_job.hpp"
#include "policy.hpp"
#include "rcs_config.hpp"
#include "session.hpp"
#include "socket_connection.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

static const uint8 kBtExtendMsgCode = 20;
static const uint8 kBtExtendAnnounceCode = 0;
static const uint8 kLocalMetadataExtendCode = 2;
static const uint32 kMetadataPieceSize = 16 * 1024; // Э��涨

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯�� 
 * ��  ��: ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��24��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtPeerConnection::BtPeerConnection(Session& session, const SocketConnectionSP& sock_conn
	, PeerType peer_type, uint64 download_bandwidth_limit, uint64 upload_bandwidth_limit)
	: PeerConnection(session, sock_conn, peer_type, download_bandwidth_limit, upload_bandwidth_limit)
	, support_metadata_extend_(false)
	, retrive_metadata_failed_(false)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��XX��XX��
 *   ���� XXX
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtPeerConnection::~BtPeerConnection()
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
void BtPeerConnection::Initialize()
{
	FsmStateSP state;

	state.reset(new BtFsmInitState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_INIT, state);

	state.reset(new BtFsmHandshakeState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_HANDSHAKE, state);

	state.reset(new BtFsmMetadataState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_METADATA, state);

	state.reset(new BtFsmBlockState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_BLOCK, state);

	state.reset(new BtFsmTransferState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_TRANSFER, state);

	state.reset(new BtFsmUploadState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_UPLOAD, state);

	state.reset(new BtFsmDownloadState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_DOWNLOAD, state);

	state.reset(new BtFsmSeedState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_SEED, state);

	state.reset(new BtFsmCloseState(fsm_.get(), this));
	fsm_->SetFsmStateObj(FSM_STATE_CLOSE, state);
}

/*-----------------------------------------------------------------------------
 * ��  ��: 
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��XX��XX��
 *   ���� XXX
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::SecondTick()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����BTЭ�����Ϣʶ����
 * ��  ��:
 * ����ֵ: ��Ϣʶ�����б�
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
std::vector<MsgRecognizerSP> BtPeerConnection::CreateMsgRecognizers()
{
	std::vector<MsgRecognizerSP> recognizers;

	recognizers.push_back(BtHandshakeMsgRecognizerSP(new BtHandshakeMsgRecognizer()));
	recognizers.push_back(BtCommonMsgRecognizerSP(new BtCommonMsgRecognizer()));

	return recognizers;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���keep-alive��Ϣ
 * ��  ��: [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ�ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteKeepAliveMsg(char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������֧����չЭ��ı���
 * ��  ��:        
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::SendExtendAnnounceMsg()
{
	LOG(INFO) << "Send BT extend announce msg";

	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	SendBuffer send_buf = session().mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	WriteExtendAnnounceMsg(send_buf.buf, send_buf.len);

	socket_connection()->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��metadata���� ���� metadata������Ϣ
 * ��  ��: [in] piece_index ��Ϣ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::SendMetadataRequestMsg(uint64 piece_index)
{
	LOG(INFO) << "Send BT metadata request msg | piece: " << piece_index;

	SendBuffer send_buf = session().mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	WriteMetadataRequestMsg(piece_index, send_buf.buf, send_buf.len);

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
void BtPeerConnection::SendMetadataDataMsg(uint64 piece_index)
{
	LOG(INFO) << "Send BT metadata data msg | piece: " << piece_index;

	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	SendBuffer send_buf = session().mem_buf_pool().AllocMsgBuffer(MSG_BUF_DATA);

	WriteMetadataDataMsg(piece_index, send_buf.buf, send_buf.len);

	socket_connection()->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����֧�ֵ���չЭ����Ϣ����ǰֻ֧��metadata��չ
 * ��  ��: [out] buf  ������
 *         [out] len  ��������С
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteExtendAnnounceMsg(char* buf, uint64& len)
{	
	len = 0; // �ȳ�ʼ��

	BtTorrentSP bt_torrent = SP_CAST(BtTorrent, torrent().lock());
	if (!bt_torrent) return;

	BencodeEntry root;
	BencodeEntry& m_entry = root["m"];
	m_entry["ut_metadata"] = kLocalMetadataExtendCode;

	// �������û��metadata����Ҫ���ʹ��ֶΣ���Ӱ��metadata��ȡ
	if (!bt_torrent->GetMetadata().empty())
	{
		root["metadata_size"] = bt_torrent->GetMetadata().size();
	}

	std::string bencode_str = Bencode(root);

	uint32 bencode_len = bencode_str.size();
	std::memcpy(buf + 4 + 1 + 1, bencode_str.c_str(), bencode_len);

	IO::write_uint32(bencode_len + 2, buf);		 // �����ܳ���
	IO::write_uint8(kBtExtendMsgCode, buf);		 // ��չЭ���
	IO::write_uint8(kBtExtendAnnounceCode, buf); // ��Э���

	len = bencode_len + 4 + 1 + 1;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Metadata������Ϣ
 * ��  ��: [in] piece_index ��Ϣ����
 *         [out] buf  ������
 *         [out] len  ��������С
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteMetadataRequestMsg(int piece_index, char* buf, uint64& len)
{
	BencodeEntry e;	
	e["msg_type"] = 0; // metadata-request
	e["piece"] = piece_index;	
	std::string bencode_str = Bencode(e);

	uint64 bencode_len = bencode_str.size();
	std::memcpy(buf + 6, bencode_str.c_str(), bencode_len);	
	
	IO::write_uint32(bencode_len + 2, buf);					// �����ܳ���
	IO::write_uint8(kBtExtendMsgCode, buf);					// ��չЭ���
	IO::write_uint8(metadata_extend_msg_.ut_metadata, buf); // metadata��Э���

	len = bencode_len + 4 + 1 + 1;

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
void BtPeerConnection::WriteMetadataDataMsg(int piece_index, char* buf, uint64& len)
{
	len = 0; // ��ʼ��һ��

	BtTorrentSP bt_torrent = SP_CAST(BtTorrent, torrent().lock());
	if (!bt_torrent) return;

	uint32 metadata_size = bt_torrent->GetMetadata().size();
	if (metadata_size == 0)
	{
		LOG(ERROR) << "No metadata avaliable";
		return;
	}

	if (static_cast<uint32>(piece_index) > metadata_size / kMetadataPieceSize)
	{
		LOG(ERROR) << "Invalid piece index | " << piece_index;
		return;
	}
	
	// ���bencode

	BencodeEntry e;	
	e["msg_type"] = 1; // metadata-data
	e["piece"] = piece_index;
	e["total_size"] = metadata_size;

	std::string bencode_str = Bencode(e);
	uint64 bencode_len = bencode_str.size();
	std::memcpy(buf + 6, bencode_str.c_str(), bencode_len);

	// ���data

	uint32 metadata_len = kMetadataPieceSize;
	if (static_cast<uint32>(piece_index) == metadata_size / kMetadataPieceSize)
	{
		metadata_len = metadata_size % kMetadataPieceSize;
	}

	const char* metadata_start = bt_torrent->GetMetadata().c_str() 
		                         + kMetadataPieceSize * piece_index;

	std::memcpy(buf + 6 + bencode_len, metadata_start, metadata_len);
	
	// ��䳤��

	IO::write_uint32(bencode_len + metadata_len + 1 + 1, buf);	// �����ܳ���
	IO::write_uint8(kBtExtendMsgCode, buf);						// ��չЭ���
	IO::write_uint8(kLocalMetadataExtendCode, buf);				// metadata��Э���

	len = bencode_len + metadata_len + 4 + 1 + 1;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������������Ϣ
 * ��  ��: [in] request ���������
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteDataRequestMsg(const PeerRequest& request, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(6)  // ��Ϣ����
        & uint32(request.piece)  // piece index
        & uint32(request.start)  // offset in the piece
        & uint32(request.length);  // block data length

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ȡ��������Ϣ
 * ��  ��: [in] pb block���� 
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteDataCancelMsg(const PieceBlock& pb, char* buf, uint64& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

    BtNetSerializer serializer(buf);

    serializer & uint8(8) //��Ϣ����
        & uint32(pb.piece_index) //piece index
        & uint32(pb.block_index) //block index
        & uint32(torrent->GetBlockSize(pb.piece_index, pb.block_index)); //block data length

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���port��Ϣ
 * ��  ��: [in] port DHT��listen port
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ�ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WritePortMsg(unsigned short port, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(9) //��Ϣ����
        & uint16(port); //DHT listen port

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ݻظ���Ϣ
 * ��  ��: [in] job block����
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteDataResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(7) //��Ϣ����
        & uint32(job->piece) //piece index
        & uint32(job->offset); //block index 
    
    memcpy(serializer->value(), job->buf, job->len); //block data
    serializer->advance(job->len);

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���hava��Ϣ
 * ��  ��: [in] piece_index piece index
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteHaveMsg(int piece_index, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(4) //��Ϣ����
        & uint32(piece_index); //piece index
    
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���choke/unchoke��Ϣ
 * ��  ��: [in] choke ��choke��Ϣ(true)����unchoke(false)��Ϣ
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ�ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteChokeMsg(bool choke, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(choke ? 0 : 1); //��Ϣ����
    
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���interested/not interested��Ϣ
 * ��  ��: [in] interested ��interested(true)����not interested(false)��Ϣ
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len ��Ϣ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteInterestMsg(bool interested, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(interested ? 2 : 3); //��Ϣ����
    
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ��չЭ����Ϣ������
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtMsgType BtPeerConnection::GetBtExtendMsgType(const char* buf, uint64 len)
{
	// ƫ��pkt_len(4) + msg_code(1)��ȡ��չЭ����Ϣ������
	uint8 extend_sub_code = *(buf + 4 + 1);

	// ����֧����չЭ�鱨�ĵ���չ��Э��Ź̶�Ϊ0
	if (extend_sub_code == 0) return BT_MSG_EXTEND_ANNOUNCE;

	// ��ǰ����ֻʹ��metadata��չЭ��
	// ����Ҫ�������������һ�����������֧��metadata��չЭ�飬���ʱsub-code
	// �������Ƕ���ģ�peer���͵�������Ϣ�е�sub-codeӦ�������Ƕ����sub-code
	// ��һ������ǣ��Է�֧��metadata��չЭ�飬peer����Ӧ��Ϣ��Ӧ���������Ѿ�
	// ��¼���Ƕ����sub-code��
	if (extend_sub_code != kLocalMetadataExtendCode
		&& extend_sub_code != metadata_extend_msg_.ut_metadata) 
	{
		LOG(ERROR) << "Unexpected extend sub-code | expect: " 
			       << metadata_extend_msg_.ut_metadata
				   << " come: " << extend_sub_code;
		return BT_MSG_UNKNOWN;
	}

	// ƫ�Ƶ�bendcode���ֿ�ʼ������Ϣ����(len + type + sub-code)
	// ��Ȼ�����ᵼ����չЭ����Ϣ���������飬����������չЭ����Ϣ�����ͺ���
	// ���ң��ڻ�ȡ��metadata���ǲ���Ҫ���»�ȡ�ģ���ˣ�������Ӱ���С
	LazyEntry root;
	if(0 != LazyBdecode(buf + 4 + 1 + 1, buf + len, root)) 
	{
		LOG(ERROR) << "Decode bencode of extend msg error";
		return BT_MSG_UNKNOWN;
	}

	uint8 msg_type = root.DictFindIntValue("msg_type", 0xFF);
	if (0xFF == msg_type)
	{
		LOG(ERROR) << "Can't find 'msg_type' in msg";
		return BT_MSG_UNKNOWN;
	}

	BC_ASSERT(msg_type < 3);
	if (msg_type == 0)
	{
		if (kLocalMetadataExtendCode == extend_sub_code)
		{
			return BT_MSG_METADATA_REQUEST;
		}
		else
		{
			LOG(WARNING) << "Invalide sub code for metadata request | " << extend_sub_code;
			return BT_MSG_UNKNOWN;
		}
	}
	else if (msg_type == 1)
	{
		return BT_MSG_METADATA_DATA;
	}
	else if (msg_type == 2)
	{
		return BT_MSG_METADATA_REJECT;
	}
	else
	{
		LOG(ERROR) << "Recevied unknown extend msg | type: " << msg_type;
		return BT_MSG_UNKNOWN;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����������ݻ�ȡ��Ϣ����
 * ��  ��: [in] buf ��Ϣ����
 *         [in] uint64 ��Ϣ���ݵĳ���
 * ����ֵ: ��Ϣ����
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtMsgType BtPeerConnection::GetMsgType(const char* buf, uint64 len)
{
    //�����ж��Ƿ���keep-alive��Ϣ
    if (NetToHost<uint32>(buf) == 0)
    {
        return BT_MSG_KEEPALIVE;
    }

    //Ȼ���ж��Ƿ���handshake��Ϣ
    if ((NetToHost<uint8>(buf) == BT_HANDSHAKE_PROTOCOL_STR_LEN) 
        && (std::strncmp(BT_HANDSHAKE_PROTOCOL_STR, buf + 1, BT_HANDSHAKE_PROTOCOL_STR_LEN) == 0))
    {
        return BT_MSG_HANDSHAKE;
    }

    // ��ȡ��Ϣ����
    BtMsgType msg_type = static_cast<BtMsgType>(NetToHost<uint8>(buf + 4)); 
	if (msg_type == kBtExtendMsgCode)
	{
		return GetBtExtendMsgType(buf, len);
	}
	else
	{		//�ж���Ϣ�Ƿ�Ϸ�
		return ((msg_type >= BT_MSG_BEGIN) && (msg_type <= BT_MSG_END)) ? msg_type : BT_MSG_UNKNOWN; 
	}   
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ut_metadata�ı���
 * ��  ��: [in] cosnt char* ��Ϣ����
 *         [in] uint64 ��Ϣ���ݵĳ���
 * ����ֵ: �����ɹ����
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseExtendAnnounceMsg(const char* buf, uint64 len, 
										      BtMetadataExtendMsg& msg)
{
	LOG(INFO) << "Parse metadata extend msg";
    
	//ƫ��len + pkt_type + sub-code
	buf += 4 + 1 + 1;
	len = len - 4 -1 -1;
	LazyEntry root;
	int res = LazyBdecode(buf, buf + len, root);
	if (res != 0 || root.Type() != LazyEntry::DICT_ENTRY)
	{
		LOG(ERROR) << "Parse bencode error";
		return false;
	}
	// Bencode ���� �鿴�Ƿ��� m �ֶ�
	LazyEntry const* messages = root.DictFind("m");
	if ((messages == nullptr) || messages->Type() != LazyEntry::DICT_ENTRY)
	{
		LOG(ERROR) << "Fail to parse metadata extend msg";
		return false;
	}
	int index = messages->DictFindIntValue("ut_metadata", -1);
	if (index == -1)
	{
		LOG(ERROR) << "Fail to parse metadata extend msg, can't find ut_metadata";
		return false;
	}

	//�˴�����Ҫ�����torrent�����ļ���С
	int metadata_size = root.DictFindIntValue("metadata_size", 0);

	msg.ut_metadata = index;
	msg.metadata_size = metadata_size;
	
	return true;
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
bool BtPeerConnection::ParseMetadataRequestMsg(const char* buf, uint64 len, 
	                                           BtMetadataRequestMsg& msg_data)
{
	// ����bencode��������(ƫ��len + pkt_type + sub-code)
	std::size_t bencode_len;
	BencodeEntry msg = Bdecode(buf + 4 + 1 + 1, buf + len, &bencode_len);
	if (msg.Type() == BencodeEntry::UNDEFINED_T)
	{
		LOG(INFO) << "Invalid metadata request message";
		return false;
	}

	// �����ȼ��豨���ǺϷ���
	msg_data.msg_type = msg["msg_type"].Integer();
	msg_data.piece = msg["piece"].Integer();

	return true;	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���� metadata ��Ϣ��data����
 *         ��Ϣ��ʽ��
 *         
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 		   [in] msg_data �������ص���Ϣ��BtMetadataMsg
 * ����ֵ: �����ɹ����
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseMetadataDataMsg(const char* buf, uint64 len, 
	                                        BtMetadataDataMsg& msg_data)
{
	//TODO: ����ж�ò�Ʋ��Ͻ�
	if (len > 17 * 1024)
	{
		LOG(ERROR) << "Metadata data message is too big";
		return false;
	}

	// ����bencode��������(ƫ��len + pkt_type + sub-code)
	std::size_t bencode_len;
	BencodeEntry msg = Bdecode(buf + 4 + 1 + 1, buf + len, &bencode_len);
	if (msg.Type() == BencodeEntry::UNDEFINED_T)
	{
		LOG(INFO) << "Invalid metadata data message";
		return false;
	}

	// �����ȼ��豨���ǺϷ���
	msg_data.msg_type = msg["msg_type"].Integer();
	msg_data.piece = msg["piece"].Integer();
	msg_data.buf = buf + 4 + 1 + 1 + bencode_len;
	msg_data.len = len - 4 - 1 - 1 - bencode_len;
	msg_data.total_size = msg["total_size"].Integer();

	return true;	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����metadata reject��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg_data ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseMetadataRejectMsg(const char* buf, size_t len, 
	                                          BtMetadataRejectMsg& msg_data)
{
	// ����bencode��������(ƫ��len + pkt_type + sub-code)
	LazyEntry root;
	int ret = LazyBdecode(buf + 4 + 1 + 1, buf + len, root);
	if (ret != 0 || root.Type() != LazyEntry::DICT_ENTRY)
	{
		LOG(ERROR)<< "Parse bencode error";
		return false;
	}

	msg_data.msg_type = root.DictFindIntValue("msg_type", 0xFFFFFFFF);
	if (msg_data.msg_type == 0xFFFFFFFF) return false;

	msg_data.piece = root.DictFindIntValue("piece", 0xFFFFFFFF);
	if (msg_data.msg_type == 0xFFFFFFFF) return false;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����have��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtHaveMsg(const char* buf, uint64 len, BtHaveMsg& msg)
{
    BtNetUnserializer(buf) & msg.piece_index;

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����bitfield��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtBitfieldMsg(const char* buf, uint64 len, BtBitfieldMsg& msg)
{
    msg.bitfield.clear();

    const char* bit_data = buf + 4 + 1; //bitfield���ݿ�ʼλ��
    uint32 bitfield_len = len - 4 - 1; //bitfield���ݳ���
    
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
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtRequestMsg(const char* buf, uint64 len, BtRequestMsg& msg)
{
    BtNetUnserializer(buf) & msg.index & msg.begin & msg.length;

    return true;
} 

/*-----------------------------------------------------------------------------
 * ��  ��: ����piece��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtPieceMsg(const char* buf, uint64 len, BtPieceMsg& msg)
{  
    const char* data = (BtNetUnserializer(buf) & msg.index & msg.begin).value();
    msg.block_data.reset(new std::string(data, buf + len - data));
    
    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����cancel��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtCancelMsg(const char* buf, uint64 len, BtCancelMsg& msg)
{
    BtNetUnserializer(buf) & msg.index & msg.begin & msg.length;

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����port��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtPortMsg(const char* buf, uint64 len, BtPortMsg& msg)
{
    BtNetUnserializer(buf) & msg.listen_port;
 
    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����handshake��Ϣ
 * ��  ��: [in] buf ��Ϣ����
 *         [in] len ��Ϣ���ݵĳ���
 *         [out] msg ���ص���Ϣ
 * ����ֵ: �Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.25
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtHandshakeMsg(const char* buf, uint64 len, BtHandshakeMsg& msg)
{
    uint8 protocol_length = NetToHost<uint8>(buf); //Э���ַ�������
    buf += 1;

    msg.protocol.resize(protocol_length); //����Э���ַ���
    std::memcpy(&msg.protocol[0], buf, protocol_length);
    buf += protocol_length;

    std::memcpy(msg.reserved, buf, 8);
    buf += 8;

    std::memcpy(msg.info_hash, buf, 20); //����info-hash
    buf += 20;

    std::memcpy(msg.peer_id, buf, 20); //����peer id 
    
    msg.extend_supported = ((msg.reserved[5] & 0x10) != 0); //�Ƿ��btЭ����չ

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����peer_id����peertype
 * ��  ��: [in]  peer_id
 * ����ֵ: ����peerclient������ PeerclientType
 * ��  ��:
 *   ʱ�� 2013��9��28��
 *   ���� vicent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
PeerClientType BtPeerConnection::GetPeerClientType( const std::string& peer_id)
{
	const char * ClentsTypeSymbol[] = {"", "BC", "XL", "BX", "UT", "SP" ,"SD"};
	std::string peer_client_type_symbol = peer_id.substr(1,2);  //����peer_id �ַ����ڶ�λ������λ�ַ�
	PeerClientType peer_client = PEER_CLIENT_UNKNOWN;

	for(uint32 i = 0; i < sizeof(ClentsTypeSymbol)/sizeof(ClentsTypeSymbol[0]); ++i)
	{
		if(std::strcmp(peer_client_type_symbol.c_str(),ClentsTypeSymbol[i]))
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
 * ��  ��: ����peer_id����peertype
 * ��  ��: ���bitfield��Ϣ
 * ��  ��: [in] piece-map bitfield��Ϣ
 *         [out] buf �����Ϣ�Ļ�����
 *         [out] len bitfield��Ϣ�ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WritePieceMapMsg(const boost::dynamic_bitset<>& piece_map, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(5);  // ��Ϣ����

    size_t bits = (7 + piece_map.size()) / 8;  // λ���ֽ���
    char* data = serializer->value();

    memset(data, 0, bits);  // ����������

    for (size_t i = 0; i < piece_map.size(); ++i)
    {
        if(piece_map[i])
        {
            data[i / 8] |= (1 << (7 - i % 8));  // ���ö�Ӧλ
        }
    }

    serializer->advance(bits);

    len = serializer.Finish();
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�choke��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_CHOKE��BT_MSG_UNCHOKE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnChokeMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received BT choke msg | " << (msg_id == BT_MSG_CHOKE);

    ReceivedChokeMsg(msg_id == BT_MSG_CHOKE);
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�interested��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_INTERESTED��BT_MSG_UNINTERESTED
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnInterestedMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received BT interest msg | " << (msg_id == BT_MSG_INTERESTED);

    ReceivedInterestMsg(msg_id == BT_MSG_INTERESTED);
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�have��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_HAVE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnHaveMsg(MsgId msg_id, const char* data, uint64 length)
{
    BtHaveMsg msg;
    ParseBtHaveMsg(data, length, msg);

	LOG(INFO) << "Received BT have piece msg | " << msg.piece_index;

    ReceivedHaveMsg(msg.piece_index);

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�bitfield��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_BITFIELD
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnBitfieldMsg(MsgId msg_id, const char* data, uint64 length)
{
    BtBitfieldMsg msg;
    ParseBtBitfieldMsg(data, length, msg); 
    ReceivedBitfieldMsg(msg.bitfield);

    LOG(INFO) << "Received BT bitfield msg : " << GetReversedBitset(msg.bitfield);

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�request��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_REQUEST
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnRequestMsg(MsgId msg_id, const char* data, uint64 length)
{
    BtRequestMsg msg; 
    ParseBtRequestMsg(data, length, msg);

    LOG(INFO) << "Received BT request msg | " << "[" << msg.index 
		      << " : " << msg.begin << " : " << msg.length << "]";

    ReceivedDataRequest(PeerRequest(msg.index, msg.begin, msg.length));

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�piece��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_PIECE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnPieceMsg(MsgId msg_id, const char* data, uint64 length)
{
    BtPieceMsg msg;
    ParseBtPieceMsg(data, length, msg);

	LOG(INFO) << "Received BT piece msg | " << "[" << msg.index 
		<< ":" << msg.begin << ":" << length << "]";

    block_data_ = msg.block_data;

    ReceivedDataResponse(PeerRequest(msg.index, msg.begin, 
        msg.block_data->size()), msg.block_data->c_str());

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�cancel��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_CANCEL
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnCancelMsg(MsgId msg_id, const char* data, uint64 length)
{
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�port��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_PORT_MSG
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnPortMsg(MsgId msg_id, const char* data, uint64 length)
{
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�keep-alive��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_KEEPALIVE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnKeepAliveMsg(MsgId msg_id, const char* data, uint64 length)
{
    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�handshake��Ϣ��Ļص����� 
 * ��  ��: [in] msg_id ��Ϣ�ţ�ΪBT_MSG_HANDSHAKE
 *         [in] data ��Ϣ��Ӧ��ָ��
 *         [in] length ��Ϣ��Ӧ�����ݳ���
 * ����ֵ: �Ƿ�ɹ��������Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.09
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnHandshakeMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received BT handshake msg";

    BtHandshakeMsg msg;
    ParseBtHandshakeMsg(data, length, msg);

    peer_id_.assign(msg.peer_id, 20);
    client_type_ = GetPeerClientType(peer_id_);

    const char* info_hash = msg.info_hash;
    ObtainedInfoHash(InfoHashSP(new BtInfoHash(std::string(info_hash, info_hash + 20))));

	// �յ��Զ˵�HandShake��Ϣ�����Ϸ���һ��HandShake��Ȼ��֪ͨ�ϲ��������
	if (socket_connection()->connection_type() == CONN_PASSIVE)
    {
        SendHandshakeMsg();
    }

    HandshakeComplete();

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
bool BtPeerConnection::OnMetadataDataMsg(MsgId msg_id, const char* data, uint64 length)
{
    LOG(INFO) << "Received metadata data msg";

	BtTorrentSP bt_torrent = SP_CAST(BtTorrent, torrent().lock());
	if (!bt_torrent) return false;

    BtMetadataDataMsg msg;
    if (!ParseMetadataDataMsg(data, length, msg))
	{
		LOG(ERROR) << "Fail to parse metadata data message";
		return false;
	}

	// ��������Ӧ���Ľ���MetadataRetriver����
	bt_torrent->metadata_retriver()->ReceivedMetadataDataMsg(msg);

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ������չ֧��Э����Ϣ����
 * ��  ��: [in] msg_id ��Ϣ��
 *         [in] data ��Ϣ����
 *         [in] len ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtPeerConnection::OnExtendAnnounceMsg(MsgId msg_id, const char* data, uint64 len)
{
    LOG(INFO) << "Received extend announce msg";

    if (!ParseExtendAnnounceMsg(data, len, metadata_extend_msg_))
	{
		LOG(ERROR) << "Parse extend announce msg error";
		return false;
	}

	// ��Ǵ�����֧��metadata��չ
	if (metadata_extend_msg_.metadata_size != 0)
	{
		support_metadata_extend_ = true;
	}

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
bool BtPeerConnection::OnMetadataRequestMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received metadata request message";
	
	BtMetadataRequestMsg msg;
	if (!ParseMetadataRequestMsg(data, length, msg))
	{
		LOG(ERROR) << "Fail to parse metadata request msg";
		return false;
	}

	SendMetadataDataMsg(msg.piece);

	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: metadata-reject��Ϣ����
 * ��  ��: [in] msg_id ��Ϣ��
 *         [in] data ��Ϣ����
 *         [in] length ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtPeerConnection::OnMetadataRejectMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received metadata reject msg";

	BtMetadataRejectMsg msg;
	if (!ParseMetadataRejectMsg(data, length, msg))
	{
		LOG(ERROR) << "Fail to parse metadata reject msg";
		return false;
	}

	BtTorrentSP bt_torrent = SP_CAST(BtTorrent, torrent().lock());
	if (!bt_torrent) return false;

	// ��������Ӧ���Ľ���MetadataRetriver����
	bt_torrent->metadata_retriver()->ReceivedMetadataRejectMsg(msg);

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
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnUnknownMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received unknown msg | " << msg_id;

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����handshake��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ��?2013.10.26
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtPeerConnection::SendHandshakeMsg()
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	LOG(INFO) << "Send BT handshake msg";

	MemBufferPool& mem_pool = torrent->session().mem_buf_pool();
	SendBuffer send_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

    char* buf = send_buf.buf;

	*buf = BT_HANDSHAKE_PROTOCOL_STR_LEN; //���Э���ַ�������
    buf += 1;

    std::memcpy(buf, BT_HANDSHAKE_PROTOCOL_STR, BT_HANDSHAKE_PROTOCOL_STR_LEN); //���Э���ַ���
    buf += BT_HANDSHAKE_PROTOCOL_STR_LEN;

    //���ñ����ֶ�
    char reserved[8] = {0};
	//�����ֶ�ǰ��λ���0x6578,�ܱ�֤��Ѹ�׿ͻ��˵��������ӡ� ��Ѹ�ף�bitcomment���������Ӷ��д��ֶ�
	reserved[0] |= 0x65;
	reserved[1] |= 0x78; 
    reserved[5] |= 0x10; //����֧��btЭ����?    reserved[7] |= 0x05; //����֧��btЭ����չ
    std::memcpy(buf, reserved, 8); //��䱣���ֶ�
    buf += 8;

    std::memcpy(buf, torrent->info_hash()->raw_data().c_str(), 20); //���20λ��info-hash
    buf += 20;

    std::memcpy(buf, BtSession::GetLocalPeerId(), 20); //���20λ��peer id
    buf += 20;

    send_buf.len = buf - send_buf.buf; //���÷������ݳ���

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
void BtPeerConnection::SendBitfieldMsg()
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	MemBufferPool& mem_pool = session().mem_buf_pool();

    PiecePicker* piece_picker = torrent->piece_picker();
	if (!piece_picker) return;

	LOG(INFO) << "Send BT bitfield msg | " 
		      << GetReversedBitset(piece_picker->GetPieceMap());

	SendBuffer send_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

	WritePieceMapMsg(piece_picker->GetPieceMap(), send_buf.buf, send_buf.len);

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
void BtPeerConnection::SendKeepAliveMsg()
{

}

/*------------------------------------------------------------------------------
 * ��  ��: ����choke/unchoke��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtPeerConnection::SendChokeMsg(bool choke)
{
	LOG(INFO) << "Send BT choke msg | flag: " << choke;

	MemBufferPool& mem_pool = session().mem_buf_pool();
	SendBuffer unchoke_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

	WriteChokeMsg(choke, unchoke_buf.buf, unchoke_buf.len);

	socket_connection()->SendData(unchoke_buf);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����interest/uninterest��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtPeerConnection::SendInterestMsg(bool interest)
{
	LOG(INFO) << "Send BT interest msg | flag: " << interest;

	MemBufferPool& mem_pool = session().mem_buf_pool();
	SendBuffer interested_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

	WriteInterestMsg(interest, interested_buf.buf, interested_buf.len);

	socket_connection()->SendData(interested_buf);
}

}
