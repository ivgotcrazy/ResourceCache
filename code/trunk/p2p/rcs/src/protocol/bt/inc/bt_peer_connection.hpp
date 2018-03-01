/*#############################################################################
 * �ļ���   : bt_peer_connection.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��13��
 * �ļ����� : BtPeerConnection������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEAD_BT_PEER_CONNECTION
#define HEAD_BT_PEER_CONNECTION

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "peer_connection.hpp"
#include "bc_assert.hpp"
#include "bencode.hpp"
#include "lazy_entry.hpp"
#include "peer.hpp"
#include "message_recombiner.hpp"
#include "protocol_msg_macro.hpp"

namespace BroadCache
{

enum BtMsgType
{
    BT_MSG_BEGIN = 0,
	BT_MSG_CHOKE = BT_MSG_BEGIN,
	BT_MSG_UNCHOKE = 1,
	BT_MSG_INTERESTED = 2,
	BT_MSG_NOTINTERESTED = 3,
	BT_MSG_HAVE = 4,
	BT_MSG_BITFIELD = 5,
	BT_MSG_REQUEST = 6,
	BT_MSG_PIECE = 7,
	BT_MSG_CANCEL = 8,
    BT_MSG_PORT = 9,
    BT_MSG_END = BT_MSG_PORT,

	BT_MSG_KEEPALIVE = 10,
	BT_MSG_HANDSHAKE = 11,

	BT_MSG_EXTEND_ANNOUNCE = 12,
	BT_MSG_METADATA_REQUEST = 13,
	BT_MSG_METADATA_DATA = 14,
	BT_MSG_METADATA_REJECT = 15,

	BT_MSG_UNKNOWN,
};

/*------------------------------------------------------------------------------
 *��  ��: bt handshake��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtHandshakeMsg
{
    std::string protocol; //Э���ַ���
    char reserved[8]; //�����ֶ�
    char info_hash[20]; //info-hash
    char peer_id[20]; //peer id
    bool extend_supported; //�Ƿ�֧��btЭ����չ
};

/*------------------------------------------------------------------------------
 *��  ��: bt have��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtHaveMsg
{
    uint32 piece_index;
};

/*------------------------------------------------------------------------------
 *��  ��: bt bitfield��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtBitfieldMsg
{
    boost::dynamic_bitset<> bitfield;
};

/*------------------------------------------------------------------------------
 *��  ��: bt request��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtRequestMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //��������ݳ���
};

/*------------------------------------------------------------------------------
 *��  ��: bt piece��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtPieceMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    boost::shared_ptr<std::string> block_data; //�����������
};

/*------------------------------------------------------------------------------
 *��  ��: bt cancel��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtCancelMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //��������ݳ���
};

/*------------------------------------------------------------------------------
 *��  ��: bt port��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtPortMsg
{
    uint16 listen_port; //���߽��շ��ڵ㣬���ͷ�֧��DHT������DHT�����Ķ˿ں���listen_port
};

/*------------------------------------------------------------------------------
 *��  ��: BtMetadata����󷵻ص�data��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtMetadataDataMsg
{
	uint8 msg_type;
	uint32 piece;
	uint32 total_size;
	uint32 len;
	const char* buf;
};

/*-----------------------------------------------------------------------------
 * ��  ��: Handshake���extend��Ϣ�����к��д�peerconnection�Ƿ�֧��metadata��ȡ
 * ��  ��: tom_liu
 * ʱ  ��: 2013.10.16
 ----------------------------------------------------------------------------*/ 
struct BtMetadataExtendMsg
{
	uint32  ut_metadata;
	uint32  metadata_size;
};

/*-----------------------------------------------------------------------------
 * ��  ��: metadata������Ϣ����ʱ��֧��
 * ��  ��: teck_zhou
 * ʱ  ��: 2013��10��27��
 ----------------------------------------------------------------------------*/ 
struct BtMetadataRequestMsg
{
	uint32 msg_type;
	uint32 piece;
};

/*-----------------------------------------------------------------------------
 * ��  ��: metadata�ܾ���Ϣ
 * ��  ��: teck_zhou
 * ʱ  ��: 2013��10��27��
 ----------------------------------------------------------------------------*/ 
struct BtMetadataRejectMsg
{
	uint32 msg_type;
	uint32 piece;
};
 
/******************************************************************************
 * ����: BTЭ�����Ӷ���
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��10��27��
 *****************************************************************************/
class BtPeerConnection : public PeerConnection
{
public:
	friend class BtFsmInitState;
	friend class BtFsmHandshakeState;
	friend class BtFsmMetadataState;
	friend class BtFsmTransferState;
	friend class BtFsmUploadState;
	friend class BtFsmDownloadState;
	friend class BtFsmCloseState;

	BtPeerConnection(Session& session, 
		             const SocketConnectionSP& sock_conn, 
					 PeerType peer_type, 
		             uint64 download_bandwidth_limit, 
					 uint64 upload_bandwidth_limi);
	~BtPeerConnection();

	BtMsgType GetMsgType(const char* buf, uint64 len);
	PeerClientType GetPeerClientType(const std::string& peer_id);
	PeerClientType peer_client_type() const { return client_type_; }

	bool support_metadata_extend() const { return support_metadata_extend_; }
	bool retrive_metadata_failed() const { return retrive_metadata_failed_; }
	void retrive_metadata_failed(bool fail) { retrive_metadata_failed_ = fail; }
	BtMetadataExtendMsg metadata_extend_msg() const { return metadata_extend_msg_; }

	// ������Ϣ
	void SendKeepAliveMsg();
	void SendHandshakeMsg();
	void SendChokeMsg(bool choke);
	void SendInterestMsg(bool interest);
	void SendBitfieldMsg();
	void SendMetadataRequestMsg(uint64 piece_index);
	void SendMetadataDataMsg(uint64 piece_index);
	void SendExtendAnnounceMsg();

	// ������Ϣ
	bool ParseBtHandshakeMsg(const char* buf, uint64 len, BtHandshakeMsg& msg_data);
	bool ParseBtHaveMsg(const char* buf, uint64 len, BtHaveMsg& msg_data);
	bool ParseBtBitfieldMsg(const char* buf, uint64 len, BtBitfieldMsg& msg_data);
	bool ParseBtRequestMsg(const char* buf, uint64 len, BtRequestMsg& msg_data);
	bool ParseBtPieceMsg(const char* buf, uint64 len, BtPieceMsg& msg_data);
	bool ParseBtCancelMsg(const char* buf, uint64 len, BtCancelMsg& msg_data);
	bool ParseBtPortMsg(const char* buf, uint64 len, BtPortMsg& msg_data);
	bool ParseHandShakeMsg(const char * buf, size_t len);
	bool ParseExtendAnnounceMsg(const char * buf, size_t len, BtMetadataExtendMsg& msg_data);
	bool ParseMetadataRequestMsg(const char* buf, uint64 len, BtMetadataRequestMsg& msg_data);
	bool ParseMetadataDataMsg(const char * buf, size_t len, BtMetadataDataMsg& msg_data);
	bool ParseMetadataRejectMsg(const char* buf, size_t len, BtMetadataRejectMsg& msg_data);

	// ������Ϣ
    bool OnChokeMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnInterestedMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnHaveMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnBitfieldMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnRequestMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnPieceMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnCancelMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnPortMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnKeepAliveMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnHandshakeMsg(MsgId msg_id, const char* data, uint64 length);
	bool OnExtendAnnounceMsg(MsgId msg_id, const char* data, uint64 length);
	bool OnMetadataRequestMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnMetadataDataMsg(MsgId msg_id, const char* data, uint64 length);
	bool OnMetadataRejectMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnUnknownMsg(MsgId msg_id, const char* data, uint64 length);

private: // ���⺯��
	virtual void Initialize() override;
	virtual void SecondTick() override;
	virtual std::vector<MsgRecognizerSP> CreateMsgRecognizers() override;
	
private: // �ڲ�����
	bool HandleExtend(const char* buf, uint64 len);
	bool ReceivedMetadata(char const* buf, int size, int piece, int total_size);
	BtMsgType GetBtExtendMsgType(const char* buf, uint64 len);

	// ������Ϣ
	void WriteKeepAliveMsg(char* buf, uint64& len);
	void WriteDataRequestMsg(const PeerRequest& request, char* buf, uint64& len);
	void WriteDataResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len);
	void WriteHaveMsg(int piece_index, char* buf, uint64& len);
	void WriteChokeMsg(bool choke, char* buf, uint64& len);
	void WriteInterestMsg(bool interest, char* buf, uint64& len);
	void WritePieceMapMsg(const boost::dynamic_bitset<>& piece_map, char* buf, uint64& len);
	void WriteDataCancelMsg(const PieceBlock& pb, char* buf, uint64& len);
	void WritePortMsg(unsigned short port, char* buf, size_t& len);
	void WriteExtendAnnounceMsg(char* buf, uint64& len);
	void WriteMetadataRequestMsg(int piece_index, char* buf, uint64& len);
	void WriteMetadataDataMsg(int piece_index, char* buf, uint64& len);

private:
	// metadata���
	bool support_metadata_extend_; // �Ƿ�֧��metadata��չЭ�� 
    bool retrive_metadata_failed_; // ����ʹ�ô����ӻ�ȡmetadataʧ�ܹ�
	BtMetadataExtendMsg metadata_extend_msg_; // metadata��չͷ��Ϣ����
	
	std::string peer_id_;
	PeerClientType client_type_;

    boost::shared_ptr<std::string> block_data_;
};

}

#endif
