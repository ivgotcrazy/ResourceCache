/*#############################################################################
 * 文件名   : pps_peer_connection.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月30日
 * 文件描述 : PpsPeerConnection类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 *描  述: pps handshake消息
 *作  者: tom_liu
 *时  间: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsHandshakeMsg
{
	uint16 length;  //报文长度
    char protocol_id[5]; //协议字段
    char info_hash[20]; //info-hash
    uint8 network_type; //
	boost::dynamic_bitset<> bitfield;
    char peer_id[16]; //peer id
};

/*------------------------------------------------------------------------------
 *描  述: pps have消息
 *作  者: tom_liu
 *时  间: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsHaveMsg
{
    uint32 piece_index;
};

/*------------------------------------------------------------------------------
 *描  述: pps bitfield消息
 *作  者: tom_liu
 *时  间: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsBitfieldMsg
{
	bool passive;
    boost::dynamic_bitset<> bitfield;
};

/*------------------------------------------------------------------------------
 *描  述: pps request消息
 *作  者: tom_liu
 *时  间: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsRequestMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //请求的数据长度
};

/*------------------------------------------------------------------------------
 *描  述: pps piece消息
 *作  者: tom_liu
 *时  间: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsPieceMsg
{
	uint16 checksum_front;  //包偏移11开始取44个字节计算
	uint16 checksum_end;	//包最后16字节进行计算
	uint16 seq_id;
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 data_len; 
    boost::shared_ptr<std::string> block_data; //块二进制数据
};

/*------------------------------------------------------------------------------
 *描  述: pps cancel消息
 *作  者: tom_liu
 *时  间: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsCancelMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //请求的数据长度
};

/*------------------------------------------------------------------------------
 *描  述: PpsMetadata请求后返回的data消息
 *作  者: tom_liu
 *时  间: 2013.12.27
 -----------------------------------------------------------------------------*/ 
struct PpsMetadataDataMsg
{
	uint32 piece;  //metadata中的piece相当于PpsPieceMsg中的block id
	uint32 len;
	const char* buf;
};

/*-----------------------------------------------------------------------------
 * 描  述: metadata请求消息，暂时不支持
 * 作  者: tom_liu
 * 时  间: 2013.12.26
 ----------------------------------------------------------------------------*/ 
struct PpsMetadataRequestMsg
{
	uint32 piece;
	uint32 len;
};

/******************************************************************************
 * 描述: Pps协议连接定义
 * 作者：tom_liu
 * 时间：2013年12月30日
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

	// 发送消息
	void SendKeepAliveMsg();
	void SendHandshakeMsg(bool reply);
	void SendBitfieldMsg(bool passive);
	void SendMetadataRequestMsg(uint64 piece_index, uint64 data_len);
	void SendMetadataDataMsg(uint64 piece_index);

	// 解析消息
	bool ParsePpsHandshakeMsg(const char* buf, uint64 len, PpsHandshakeMsg& msg_data);
	bool ParsePpsHaveMsg(const char* buf, uint64 len, PpsHaveMsg& msg_data);
	bool ParsePpsBitfieldMsg(const char* buf, uint64 len, PpsBitfieldMsg& msg_data);
	bool ParsePpsRequestMsg(const char* buf, uint64 len, PpsRequestMsg& msg_data);
	bool ParsePpsPieceMsg(const char* buf, uint64 len, PpsPieceMsg& msg_data);
	bool ParseHandShakeMsg(const char * buf, size_t len);
	bool ParseMetadataRequestMsg(const PpsRequestMsg& msg, PpsMetadataRequestMsg& msg_data);
	bool ParseMetadataDataMsg(const PpsPieceMsg& msg, PpsMetadataDataMsg& msg_data);
	bool ParsePpsExtendedRequestMsg(const char* buf, uint64 len, PpsRequestMsg& msg);

	// 处理消息
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
private: // 虚拟函数
	virtual void Initialize() override;
	virtual void SecondTick() override;
	virtual std::vector<MsgRecognizerSP> CreateMsgRecognizers() override;
	
private: // 内部函数
	// 构造消息
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
	// metadata相关
	bool support_metadata_extend_; // 是否支持metadata扩展协议,在处理handshake时进行判断
    bool retrive_metadata_failed_; // 曾经使用此连接获取metadata失败过
	
	std::string peer_id_;
	PeerClientType client_type_;

    boost::shared_ptr<std::string> block_data_;

	bool handshake_complete_;

	uint32 seq_id_;  //序列标识

	bool passive_;
};

}

#endif

