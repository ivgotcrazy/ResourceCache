/*#############################################################################
 * �ļ���   : pps_peer_connection.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��30��
 * �ļ����� : PpsPeerConnection������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEAD_PPS_PEER_CONNECTION
#define HEAD_PPS_PEER_CONNECTION

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "peer_connection.hpp"
#include "bc_assert.hpp"
#include "bencode.hpp"
#include "lazy_entry.hpp"
#include "peer.hpp"
#include "message_recombiner.hpp"
#include "protocol_msg_macro.hpp"
#include "pps_pub.hpp"

namespace BroadCache
{
/*------------------------------------------------------------------------------
 *��  ��: pps handshake��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsHandshakeMsg
{
	uint16 length;  //���ĳ���
    char protocol_id[5]; //Э���ֶ�
    char info_hash[20]; //info-hash
    uint8 network_type; //
	boost::dynamic_bitset<> bitfield;
    char peer_id[16]; //peer id
};

/*------------------------------------------------------------------------------
 *��  ��: pps have��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsHaveMsg
{
    uint32 piece_index;
};

/*------------------------------------------------------------------------------
 *��  ��: pps bitfield��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsBitfieldMsg
{
	bool passive;
    boost::dynamic_bitset<> bitfield;
};

/*------------------------------------------------------------------------------
 *��  ��: pps request��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsRequestMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //��������ݳ���
};

/*------------------------------------------------------------------------------
 *��  ��: pps piece��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsPieceMsg
{
	uint16 checksum_front;  //��ƫ��11��ʼȡ44���ֽڼ���
	uint16 checksum_end;	//�����16�ֽڽ��м���
	uint16 seq_id;
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 data_len; 
    boost::shared_ptr<std::string> block_data; //�����������
};

/*------------------------------------------------------------------------------
 *��  ��: pps cancel��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsCancelMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //��������ݳ���
};

/*------------------------------------------------------------------------------
 *��  ��: PpsMetadata����󷵻ص�data��Ϣ
 *��  ��: tom_liu
 *ʱ  ��: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsMetadataDataMsg
{
	uint32 piece;  //metadata�е�piece�൱��PpsPieceMsg�е�block id
	uint32 len;
	const char* buf;
};

/*-----------------------------------------------------------------------------
 * ��  ��: metadata������Ϣ����ʱ��֧��
 * ��  ��: tom_liu
 * ʱ  ��: 2013.12.26
 ----------------------------------------------------------------------------*/ 
struct PpsMetadataRequestMsg
{
	uint32 piece;
	uint32 len;
};

/******************************************************************************
 * ����: PpsЭ�����Ӷ���
 * ���ߣ�tom_liu
 * ʱ�䣺2013��12��30��
 *****************************************************************************/
class PpsPeerConnection : public PeerConnection
{
public:
	friend class PpsFsmInitState;
	friend class PpsFsmHandshakeState;
	friend class PpsFsmMetadataState;
	friend class PpsFsmTransferState;
	friend class PpsFsmUploadState;
	friend class PpsFsmDownloadState;
	friend class PpsFsmCloseState;

	PpsPeerConnection(Session& session, 
		             const SocketConnectionSP& sock_conn, 
					 PeerType peer_type, 
		             uint64 download_bandwidth_limit, 
					 uint64 upload_bandwidth_limi);
	~PpsPeerConnection();

	PpsMsgType GetMsgType(const char* buf, uint64 len);
	PeerClientType GetPeerClientType(const std::string& peer_id);
	PeerClientType peer_client_type() const { return client_type_; }

	bool support_metadata_extend() const { return support_metadata_extend_; }
	bool retrive_metadata_failed() const { return retrive_metadata_failed_; }
	void retrive_metadata_failed(bool fail) { retrive_metadata_failed_ = fail; }

	// ������Ϣ
	void SendKeepAliveMsg();
	void SendHandshakeMsg(bool reply);
	void SendBitfieldMsg(bool passive);
	void SendMetadataRequestMsg(uint64 piece_index, uint64 data_len);
	void SendMetadataDataMsg(uint64 piece_index);

	// ������Ϣ
	bool ParsePpsHandshakeMsg(const char* buf, uint64 len, PpsHandshakeMsg& msg_data);
	bool ParsePpsHaveMsg(const char* buf, uint64 len, PpsHaveMsg& msg_data);
	bool ParsePpsBitfieldMsg(const char* buf, uint64 len, PpsBitfieldMsg& msg_data);
	bool ParsePpsRequestMsg(const char* buf, uint64 len, PpsRequestMsg& msg_data);
	bool ParsePpsPieceMsg(const char* buf, uint64 len, PpsPieceMsg& msg_data);
	bool ParseHandShakeMsg(const char * buf, size_t len);
	bool ParseMetadataRequestMsg(const PpsRequestMsg& msg, PpsMetadataRequestMsg& msg_data);
	bool ParseMetadataDataMsg(const PpsPieceMsg& msg, PpsMetadataDataMsg& msg_data);
	bool ParsePpsExtendedRequestMsg(const char* buf, uint64 len, PpsRequestMsg& msg);

	// ������Ϣ
    bool OnChokeMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnHaveMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnBitfieldMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnRequestMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnPieceMsg(MsgId msg_id, const char* data, uint64 length);
	bool OnExtendedRequestMsg(MsgId msg_id, const char* data, uint64 length);
	//bool OnExtendedPieceMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnKeepAliveMsg(MsgId msg_id, const char* data, uint64 length);
    bool OnHandshakeMsg(MsgId msg_id, const char* data, uint64 length);
	bool OnMetadataRequestMsg(const PpsRequestMsg& request_msg);
    bool OnMetadataDataMsg(const PpsPieceMsg& piece_msg);
    bool OnUnknownMsg(MsgId msg_id, const char* data, uint64 length);

	void ConstructBitfieldMsg(bool passive, char* buf, uint64& len);

	bool handshake_complete() { return handshake_complete_; }
private: // ���⺯��
	virtual void Initialize() override;
	virtual void SecondTick() override;
	virtual std::vector<MsgRecognizerSP> CreateMsgRecognizers() override;
	
private: // �ڲ�����
	// ������Ϣ
	void WriteKeepAliveMsg(char* buf, uint64& len);
	void WriteDataRequestMsg(const PeerRequest& request, char* buf, uint64& len);
	void WriteDataExtendedRequestMsg(const PeerRequest& request, char* buf, uint64& len);
	void WriteDataResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len);
	void WriteDataExtendedResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len);
	void WriteHaveMsg(int piece_index, char* buf, uint64& len);
	void WritePieceMapMsg(const boost::dynamic_bitset<>& piece_map, char* buf, 
										uint64& len, bool passive);
	void WriteMetadataRequestMsg(int piece_index, uint64 data_len, char* buf, uint64& len);
	void WriteMetadataDataMsg(int piece_index, char* buf, uint64& len);

	void SetPieceMap(char* buf, uint32& len);

private:
	// metadata���
	bool support_metadata_extend_; // �Ƿ�֧��metadata��չЭ��,�ڴ���handshakeʱ�����ж�
    bool retrive_metadata_failed_; // ����ʹ�ô����ӻ�ȡmetadataʧ�ܹ�
	
	std::string peer_id_;
	PeerClientType client_type_;

    boost::shared_ptr<std::string> block_data_;

	bool handshake_complete_;

	uint32 seq_id_;  //���б�ʶ

	bool passive_;
};

}

#endif

