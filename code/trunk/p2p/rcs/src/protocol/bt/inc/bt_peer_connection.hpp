/*#############################################################################
 * 文件名   : bt_peer_connection.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月13日
 * 文件描述 : BtPeerConnection类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 *描  述: bt handshake消息
 *作  者: rosan
 *时  间: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtHandshakeMsg
{
    std::string protocol; //协议字符串
    char reserved[8]; //保留字段
    char info_hash[20]; //info-hash
    char peer_id[20]; //peer id
    bool extend_supported; //是否支持bt协议扩展
};

/*------------------------------------------------------------------------------
 *描  述: bt have消息
 *作  者: rosan
 *时  间: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtHaveMsg
{
    uint32 piece_index;
};

/*------------------------------------------------------------------------------
 *描  述: bt bitfield消息
 *作  者: rosan
 *时  间: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtBitfieldMsg
{
    boost::dynamic_bitset<> bitfield;
};

/*------------------------------------------------------------------------------
 *描  述: bt request消息
 *作  者: rosan
 *时  间: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtRequestMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //请求的数据长度
};

/*------------------------------------------------------------------------------
 *描  述: bt piece消息
 *作  者: rosan
 *时  间: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtPieceMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    boost::shared_ptr<std::string> block_data; //块二进制数据
};

/*------------------------------------------------------------------------------
 *描  述: bt cancel消息
 *作  者: rosan
 *时  间: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtCancelMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //请求的数据长度
};

/*------------------------------------------------------------------------------
 *描  述: bt port消息
 *作  者: rosan
 *时  间: 2013.10.16
 -----------------------------------------------------------------------------*/ 
struct BtPortMsg
{
    uint16 listen_port; //告诉接收方节点，发送方支持DHT，并且DHT监听的端口号是listen_port
};

/*------------------------------------------------------------------------------
 *描  述: BtMetadata请求后返回的data消息
 *作  者: tom_liu
 *时  间: 2013.10.16
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
 * 描  述: Handshake后的extend消息，其中含有此peerconnection是否支持metadata获取
 * 作  者: tom_liu
 * 时  间: 2013.10.16
 ----------------------------------------------------------------------------*/ 
struct BtMetadataExtendMsg
{
	uint32  ut_metadata;
	uint32  metadata_size;
};

/*-----------------------------------------------------------------------------
 * 描  述: metadata请求消息，暂时不支持
 * 作  者: teck_zhou
 * 时  间: 2013年10月27日
 ----------------------------------------------------------------------------*/ 
struct BtMetadataRequestMsg
{
	uint32 msg_type;
	uint32 piece;
};

/*-----------------------------------------------------------------------------
 * 描  述: metadata拒绝消息
 * 作  者: teck_zhou
 * 时  间: 2013年10月27日
 ----------------------------------------------------------------------------*/ 
struct BtMetadataRejectMsg
{
	uint32 msg_type;
	uint32 piece;
};
 
/******************************************************************************
 * 描述: BT协议连接定义
 * 作者：teck_zhou
 * 时间：2013年10月27日
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

	// 发送消息
	void SendKeepAliveMsg();
	void SendHandshakeMsg();
	void SendChokeMsg(bool choke);
	void SendInterestMsg(bool interest);
	void SendBitfieldMsg();
	void SendMetadataRequestMsg(uint64 piece_index);
	void SendMetadataDataMsg(uint64 piece_index);
	void SendExtendAnnounceMsg();

	// 解析消息
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

	// 处理消息
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

private: // 虚拟函数
	virtual void Initialize() override;
	virtual void SecondTick() override;
	virtual std::vector<MsgRecognizerSP> CreateMsgRecognizers() override;
	
private: // 内部函数
	bool HandleExtend(const char* buf, uint64 len);
	bool ReceivedMetadata(char const* buf, int size, int piece, int total_size);
	BtMsgType GetBtExtendMsgType(const char* buf, uint64 len);

	// 构造消息
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
	// metadata相关
	bool support_metadata_extend_; // 是否支持metadata扩展协议 
    bool retrive_metadata_failed_; // 曾经使用此连接获取metadata失败过
	BtMetadataExtendMsg metadata_extend_msg_; // metadata扩展头消息数据
	
	std::string peer_id_;
	PeerClientType client_type_;

    boost::shared_ptr<std::string> block_data_;
};

}

#endif
