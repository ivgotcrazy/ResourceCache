/*#############################################################################
 * 文件名   : bt_peer_connection.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月13日
 * 文件描述 : BtPeerConnection类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
static const uint32 kMetadataPieceSize = 16 * 1024; // 协议规定

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数 
 * 参  数: 略
 * 返回值:
 * 修  改:
 *   时间 2013年09月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
BtPeerConnection::BtPeerConnection(Session& session, const SocketConnectionSP& sock_conn
	, PeerType peer_type, uint64 download_bandwidth_limit, uint64 upload_bandwidth_limit)
	: PeerConnection(session, sock_conn, peer_type, download_bandwidth_limit, upload_bandwidth_limit)
	, support_metadata_extend_(false)
	, retrive_metadata_failed_(false)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年XX月XX日
 *   作者 XXX
 *   描述 创建
 ----------------------------------------------------------------------------*/
BtPeerConnection::~BtPeerConnection()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化状态机
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月24日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年XX月XX日
 *   作者 XXX
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::SecondTick()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建BT协议的消息识别器
 * 参  数:
 * 返回值: 消息识别器列表
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
std::vector<MsgRecognizerSP> BtPeerConnection::CreateMsgRecognizers()
{
	std::vector<MsgRecognizerSP> recognizers;

	recognizers.push_back(BtHandshakeMsgRecognizerSP(new BtHandshakeMsgRecognizer()));
	recognizers.push_back(BtCommonMsgRecognizerSP(new BtCommonMsgRecognizer()));

	return recognizers;
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充keep-alive消息
 * 参  数: [out] buf 填充消息的缓冲区
 *         [out] len 消息的长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteKeepAliveMsg(char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送宣告支持扩展协议的报文
 * 参  数:        
 * 返回值: 
 * 修  改:
 *   时间 2013.09.25
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 供metadata调用 发送 metadata请求消息
 * 参  数: [in] piece_index 消息索引
 * 返回值: 
 * 修  改:
 *   时间 2013.09.25
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::SendMetadataRequestMsg(uint64 piece_index)
{
	LOG(INFO) << "Send BT metadata request msg | piece: " << piece_index;

	SendBuffer send_buf = session().mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	WriteMetadataRequestMsg(piece_index, send_buf.buf, send_buf.len);

	socket_connection()->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送metadata数据响应消息
 * 参  数: [in] piece_index 消息索引
 * 返回值: 
 * 修  改:
 *   时间 2013年11月23日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 构造支持的扩展协议消息，当前只支持metadata扩展
 * 参  数: [out] buf  缓冲区
 *         [out] len  缓冲区大小
 * 返回值: 
 * 修  改:
 *   时间 2013.09.25
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteExtendAnnounceMsg(char* buf, uint64& len)
{	
	len = 0; // 先初始化

	BtTorrentSP bt_torrent = SP_CAST(BtTorrent, torrent().lock());
	if (!bt_torrent) return;

	BencodeEntry root;
	BencodeEntry& m_entry = root["m"];
	m_entry["ut_metadata"] = kLocalMetadataExtendCode;

	// 如果本端没有metadata，则不要发送此字段，不影响metadata获取
	if (!bt_torrent->GetMetadata().empty())
	{
		root["metadata_size"] = bt_torrent->GetMetadata().size();
	}

	std::string bencode_str = Bencode(root);

	uint32 bencode_len = bencode_str.size();
	std::memcpy(buf + 4 + 1 + 1, bencode_str.c_str(), bencode_len);

	IO::write_uint32(bencode_len + 2, buf);		 // 报文总长度
	IO::write_uint8(kBtExtendMsgCode, buf);		 // 扩展协议号
	IO::write_uint8(kBtExtendAnnounceCode, buf); // 子协议号

	len = bencode_len + 4 + 1 + 1;
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送Metadata请求消息
 * 参  数: [in] piece_index 消息索引
 *         [out] buf  缓冲区
 *         [out] len  缓冲区大小
 * 返回值: 
 * 修  改:
 *   时间 2013.09.25
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteMetadataRequestMsg(int piece_index, char* buf, uint64& len)
{
	BencodeEntry e;	
	e["msg_type"] = 0; // metadata-request
	e["piece"] = piece_index;	
	std::string bencode_str = Bencode(e);

	uint64 bencode_len = bencode_str.size();
	std::memcpy(buf + 6, bencode_str.c_str(), bencode_len);	
	
	IO::write_uint32(bencode_len + 2, buf);					// 报文总长度
	IO::write_uint8(kBtExtendMsgCode, buf);					// 扩展协议号
	IO::write_uint8(metadata_extend_msg_.ut_metadata, buf); // metadata子协议号

	len = bencode_len + 4 + 1 + 1;

    return ;
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送Metadata数据响应消息
 * 参  数: [in] piece_index 消息索引
 *         [out] buf  缓冲区
 *         [out] len  缓冲区大小
 * 返回值: 
 * 修  改:
 *   时间 2013年11月23日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteMetadataDataMsg(int piece_index, char* buf, uint64& len)
{
	len = 0; // 初始化一把

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
	
	// 填充bencode

	BencodeEntry e;	
	e["msg_type"] = 1; // metadata-data
	e["piece"] = piece_index;
	e["total_size"] = metadata_size;

	std::string bencode_str = Bencode(e);
	uint64 bencode_len = bencode_str.size();
	std::memcpy(buf + 6, bencode_str.c_str(), bencode_len);

	// 填充data

	uint32 metadata_len = kMetadataPieceSize;
	if (static_cast<uint32>(piece_index) == metadata_size / kMetadataPieceSize)
	{
		metadata_len = metadata_size % kMetadataPieceSize;
	}

	const char* metadata_start = bt_torrent->GetMetadata().c_str() 
		                         + kMetadataPieceSize * piece_index;

	std::memcpy(buf + 6 + bencode_len, metadata_start, metadata_len);
	
	// 填充长度

	IO::write_uint32(bencode_len + metadata_len + 1 + 1, buf);	// 报文总长度
	IO::write_uint8(kBtExtendMsgCode, buf);						// 扩展协议号
	IO::write_uint8(kLocalMetadataExtendCode, buf);				// metadata子协议号

	len = bencode_len + metadata_len + 4 + 1 + 1;
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充请求数据消息
 * 参  数: [in] request 数据请求包
 *         [out] buf 填充消息的缓冲区
 *         [out] len 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteDataRequestMsg(const PeerRequest& request, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(6)  // 消息类型
        & uint32(request.piece)  // piece index
        & uint32(request.start)  // offset in the piece
        & uint32(request.length);  // block data length

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充取消数据消息
 * 参  数: [in] pb block索引 
 *         [out] buf 填充消息的缓冲区
 *         [out] len 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteDataCancelMsg(const PieceBlock& pb, char* buf, uint64& len)
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

    BtNetSerializer serializer(buf);

    serializer & uint8(8) //消息类型
        & uint32(pb.piece_index) //piece index
        & uint32(pb.block_index) //block index
        & uint32(torrent->GetBlockSize(pb.piece_index, pb.block_index)); //block data length

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充port消息
 * 参  数: [in] port DHT的listen port
 *         [out] buf 填充消息的缓冲区
 *         [out] len 消息的长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WritePortMsg(unsigned short port, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(9) //消息类型
        & uint16(port); //DHT listen port

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充数据回复消息
 * 参  数: [in] job block数据
 *         [out] buf 填充消息的缓冲区
 *         [out] len 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteDataResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(7) //消息类型
        & uint32(job->piece) //piece index
        & uint32(job->offset); //block index 
    
    memcpy(serializer->value(), job->buf, job->len); //block data
    serializer->advance(job->len);

    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充hava消息
 * 参  数: [in] piece_index piece index
 *         [out] buf 填充消息的缓冲区
 *         [out] len 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteHaveMsg(int piece_index, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(4) //消息类型
        & uint32(piece_index); //piece index
    
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充choke/unchoke消息
 * 参  数: [in] choke 是choke消息(true)还是unchoke(false)消息
 *         [out] buf 填充消息的缓冲区
 *         [out] len 消息的长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteChokeMsg(bool choke, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(choke ? 0 : 1); //消息类型
    
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 填充interested/not interested消息
 * 参  数: [in] interested 是interested(true)还是not interested(false)消息
 *         [out] buf 填充消息的缓冲区
 *         [out] len 消息长度
 * 返回值: 
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WriteInterestMsg(bool interested, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(interested ? 2 : 3); //消息类型
    
    len = serializer.Finish();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取扩展协议消息子类型
 * 参  数: [in] buf 消息数据
 *         [in] len 消息长度
 * 返回值: 
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
BtMsgType BtPeerConnection::GetBtExtendMsgType(const char* buf, uint64 len)
{
	// 偏移pkt_len(4) + msg_code(1)获取扩展协议消息子类型
	uint8 extend_sub_code = *(buf + 4 + 1);

	// 宣告支持扩展协议报文的扩展子协议号固定为0
	if (extend_sub_code == 0) return BT_MSG_EXTEND_ANNOUNCE;

	// 当前我们只使用metadata扩展协议
	// 这里要考虑两种情况，一种情况是我们支持metadata扩展协议，则此时sub-code
	// 是由我们定义的，peer发送的请求消息中的sub-code应该填我们定义的sub-code
	// 另一种情况是，对方支持metadata扩展协议，peer的响应消息中应该填我们已经
	// 记录他们定义的sub-code。
	if (extend_sub_code != kLocalMetadataExtendCode
		&& extend_sub_code != metadata_extend_msg_.ut_metadata) 
	{
		LOG(ERROR) << "Unexpected extend sub-code | expect: " 
			       << metadata_extend_msg_.ut_metadata
				   << " come: " << extend_sub_code;
		return BT_MSG_UNKNOWN;
	}

	// 偏移到bendcode部分开始解析消息内容(len + type + sub-code)
	// 虽然这样会导致扩展协议消息被解析两遍，但是由于扩展协议消息本来就很少
	// 而且，在获取到metadata后是不需要重新获取的，因此，对性能影响很小
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
 * 描  述: 根据数据内容获取消息类型
 * 参  数: [in] buf 消息数据
 *         [in] uint64 消息数据的长度
 * 返回值: 消息类型
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
BtMsgType BtPeerConnection::GetMsgType(const char* buf, uint64 len)
{
    //首先判断是否是keep-alive消息
    if (NetToHost<uint32>(buf) == 0)
    {
        return BT_MSG_KEEPALIVE;
    }

    //然后判断是否是handshake消息
    if ((NetToHost<uint8>(buf) == BT_HANDSHAKE_PROTOCOL_STR_LEN) 
        && (std::strncmp(BT_HANDSHAKE_PROTOCOL_STR, buf + 1, BT_HANDSHAKE_PROTOCOL_STR_LEN) == 0))
    {
        return BT_MSG_HANDSHAKE;
    }

    // 获取消息类型
    BtMsgType msg_type = static_cast<BtMsgType>(NetToHost<uint8>(buf + 4)); 
	if (msg_type == kBtExtendMsgCode)
	{
		return GetBtExtendMsgType(buf, len);
	}
	else
	{		//判断消息是否合法
		return ((msg_type >= BT_MSG_BEGIN) && (msg_type <= BT_MSG_END)) ? msg_type : BT_MSG_UNKNOWN; 
	}   
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析带ut_metadata的报文
 * 参  数: [in] cosnt char* 消息数据
 *         [in] uint64 消息数据的长度
 * 返回值: 解析成功与否
 * 修  改:
 *   时间 2013.09.25
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseExtendAnnounceMsg(const char* buf, uint64 len, 
										      BtMetadataExtendMsg& msg)
{
	LOG(INFO) << "Parse metadata extend msg";
    
	//偏移len + pkt_type + sub-code
	buf += 4 + 1 + 1;
	len = len - 4 -1 -1;
	LazyEntry root;
	int res = LazyBdecode(buf, buf + len, root);
	if (res != 0 || root.Type() != LazyEntry::DICT_ENTRY)
	{
		LOG(ERROR) << "Parse bencode error";
		return false;
	}
	// Bencode 解码 查看是否有 m 字段
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

	//此处设置要请求的torrent种子文件大小
	int metadata_size = root.DictFindIntValue("metadata_size", 0);

	msg.ut_metadata = index;
	msg.metadata_size = metadata_size;
	
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析metadata请求消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 		   [in] msg_data 解析返回的消息
 * 返回值: 解析成功与否
 * 修  改:
 *   时间 2013年11月23日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseMetadataRequestMsg(const char* buf, uint64 len, 
	                                           BtMetadataRequestMsg& msg_data)
{
	// 解析bencode部分数据(偏移len + pkt_type + sub-code)
	std::size_t bencode_len;
	BencodeEntry msg = Bdecode(buf + 4 + 1 + 1, buf + len, &bencode_len);
	if (msg.Type() == BencodeEntry::UNDEFINED_T)
	{
		LOG(INFO) << "Invalid metadata request message";
		return false;
	}

	// 这里先假设报文是合法的
	msg_data.msg_type = msg["msg_type"].Integer();
	msg_data.piece = msg["piece"].Integer();

	return true;	
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析 metadata 消息的data返回
 *         消息格式：
 *         
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 		   [in] msg_data 解析返回的消息成BtMetadataMsg
 * 返回值: 解析成功与否
 * 修  改:
 *   时间 2013.09.25
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseMetadataDataMsg(const char* buf, uint64 len, 
	                                        BtMetadataDataMsg& msg_data)
{
	//TODO: 这个判断貌似不严谨
	if (len > 17 * 1024)
	{
		LOG(ERROR) << "Metadata data message is too big";
		return false;
	}

	// 解析bencode部分数据(偏移len + pkt_type + sub-code)
	std::size_t bencode_len;
	BencodeEntry msg = Bdecode(buf + 4 + 1 + 1, buf + len, &bencode_len);
	if (msg.Type() == BencodeEntry::UNDEFINED_T)
	{
		LOG(INFO) << "Invalid metadata data message";
		return false;
	}

	// 这里先假设报文是合法的
	msg_data.msg_type = msg["msg_type"].Integer();
	msg_data.piece = msg["piece"].Integer();
	msg_data.buf = buf + 4 + 1 + 1 + bencode_len;
	msg_data.len = len - 4 - 1 - 1 - bencode_len;
	msg_data.total_size = msg["total_size"].Integer();

	return true;	
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析metadata reject消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg_data 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseMetadataRejectMsg(const char* buf, size_t len, 
	                                          BtMetadataRejectMsg& msg_data)
{
	// 解析bencode部分数据(偏移len + pkt_type + sub-code)
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
 * 描  述: 解析have消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtHaveMsg(const char* buf, uint64 len, BtHaveMsg& msg)
{
    BtNetUnserializer(buf) & msg.piece_index;

    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析bitfield消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtBitfieldMsg(const char* buf, uint64 len, BtBitfieldMsg& msg)
{
    msg.bitfield.clear();

    const char* bit_data = buf + 4 + 1; //bitfield数据开始位置
    uint32 bitfield_len = len - 4 - 1; //bitfield数据长度
    
    uint32 i = 0;
    while (i < bitfield_len)
    {
        char ch = bit_data[i]; //获取那一位对应的字节
        for (int j = 7; j >= 0; --j)
        {
            msg.bitfield.push_back((ch & (1 << j)) != 0);
        }
        ++i;
    }

    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析request消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtRequestMsg(const char* buf, uint64 len, BtRequestMsg& msg)
{
    BtNetUnserializer(buf) & msg.index & msg.begin & msg.length;

    return true;
} 

/*-----------------------------------------------------------------------------
 * 描  述: 解析piece消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtPieceMsg(const char* buf, uint64 len, BtPieceMsg& msg)
{  
    const char* data = (BtNetUnserializer(buf) & msg.index & msg.begin).value();
    msg.block_data.reset(new std::string(data, buf + len - data));
    
    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析cancel消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtCancelMsg(const char* buf, uint64 len, BtCancelMsg& msg)
{
    BtNetUnserializer(buf) & msg.index & msg.begin & msg.length;

    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析port消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtPortMsg(const char* buf, uint64 len, BtPortMsg& msg)
{
    BtNetUnserializer(buf) & msg.listen_port;
 
    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析handshake消息
 * 参  数: [in] buf 消息数据
 *         [in] len 消息数据的长度
 *         [out] msg 返回的消息
 * 返回值: 是否成功
 * 修  改:
 *   时间 2013.09.25
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtPeerConnection::ParseBtHandshakeMsg(const char* buf, uint64 len, BtHandshakeMsg& msg)
{
    uint8 protocol_length = NetToHost<uint8>(buf); //协议字符串长度
    buf += 1;

    msg.protocol.resize(protocol_length); //设置协议字符串
    std::memcpy(&msg.protocol[0], buf, protocol_length);
    buf += protocol_length;

    std::memcpy(msg.reserved, buf, 8);
    buf += 8;

    std::memcpy(msg.info_hash, buf, 20); //设置info-hash
    buf += 20;

    std::memcpy(msg.peer_id, buf, 20); //设置peer id 
    
    msg.extend_supported = ((msg.reserved[5] & 0x10) != 0); //是否С謆t协议扩展

    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析peer_id返回peertype
 * 参  数: [in]  peer_id
 * 返回值: 返回peerclient的类型 PeerclientType
 * 修  改:
 *   时间 2013年9月28日
 *   作者 vicent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
PeerClientType BtPeerConnection::GetPeerClientType( const std::string& peer_id)
{
	const char * ClentsTypeSymbol[] = {"", "BC", "XL", "BX", "UT", "SP" ,"SD"};
	std::string peer_client_type_symbol = peer_id.substr(1,2);  //解析peer_id 字符串第二位到第三位字符
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
 * 描  述: 解析peer_id返回peertype
 * 描  述: 填充bitfield消息
 * 参  数: [in] piece-map bitfield信息
 *         [out] buf 填充消息的缓冲区
 *         [out] len bitfield消息的长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.30
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtPeerConnection::WritePieceMapMsg(const boost::dynamic_bitset<>& piece_map, char* buf, uint64& len)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(5);  // 消息类型

    size_t bits = (7 + piece_map.size()) / 8;  // 位的字节数
    char* data = serializer->value();

    memset(data, 0, bits);  // 将数据清零

    for (size_t i = 0; i < piece_map.size(); ++i)
    {
        if(piece_map[i])
        {
            data[i / 8] |= (1 << (7 - i % 8));  // 设置对应位
        }
    }

    serializer->advance(bits);

    len = serializer.Finish();
}

/*------------------------------------------------------------------------------
 * 描  述: 收到choke消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_CHOKE或BT_MSG_UNCHOKE
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnChokeMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received BT choke msg | " << (msg_id == BT_MSG_CHOKE);

    ReceivedChokeMsg(msg_id == BT_MSG_CHOKE);
    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 收到interested消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_INTERESTED或BT_MSG_UNINTERESTED
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnInterestedMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received BT interest msg | " << (msg_id == BT_MSG_INTERESTED);

    ReceivedInterestMsg(msg_id == BT_MSG_INTERESTED);
    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 收到have消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_HAVE
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 收到bitfield消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_BITFIELD
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 收到request消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_REQUEST
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 收到piece消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_PIECE
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 收到cancel消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_CANCEL
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnCancelMsg(MsgId msg_id, const char* data, uint64 length)
{
    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 收到port消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_PORT_MSG
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnPortMsg(MsgId msg_id, const char* data, uint64 length)
{
    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 收到keep-alive消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_KEEPALIVE
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnKeepAliveMsg(MsgId msg_id, const char* data, uint64 length)
{
    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 收到handshake消息后的回调函数 
 * 参  数: [in] msg_id 消息号，为BT_MSG_HANDSHAKE
 *         [in] data 消息对应的指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
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

	// 收到对端的HandShake消息后，马上发送一个HandShake，然后通知上层握手完成
	if (socket_connection()->connection_type() == CONN_PASSIVE)
    {
        SendHandshakeMsg();
    }

    HandshakeComplete();

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: metadata数据响应消息处理
 * 参  数: [in] msg_id 消息号
 *         [in] data 消息数据
 *         [in] len 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
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

	// 将数据响应报文交给MetadataRetriver处理
	bt_torrent->metadata_retriver()->ReceivedMetadataDataMsg(msg);

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 宣告扩展支持协议消息处理
 * 参  数: [in] msg_id 消息号
 *         [in] data 消息数据
 *         [in] len 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtPeerConnection::OnExtendAnnounceMsg(MsgId msg_id, const char* data, uint64 len)
{
    LOG(INFO) << "Received extend announce msg";

    if (!ParseExtendAnnounceMsg(data, len, metadata_extend_msg_))
	{
		LOG(ERROR) << "Parse extend announce msg error";
		return false;
	}

	// 标记此连接支持metadata扩展
	if (metadata_extend_msg_.metadata_size != 0)
	{
		support_metadata_extend_ = true;
	}

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 请求metadata数据消息处理
 * 参  数: [in] msg_id 消息号
 *         [in] data 消息数据
 *         [in] length 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: metadata-reject消息处理
 * 参  数: [in] msg_id 消息号
 *         [in] data 消息数据
 *         [in] length 消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
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

	// 将数据响应报文交给MetadataRetriver处理
	bt_torrent->metadata_retriver()->ReceivedMetadataRejectMsg(msg);

	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 当收到不能识别的bt消息后的处理函数
 * 参  数: [in] msg_id 消息号
 *         [in] data 消息对应的数据指针
 *         [in] length 消息对应的数据长度
 * 返回值: 是否成功处理此消息
 * 修  改:
 *   时间 2013.11.09
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtPeerConnection::OnUnknownMsg(MsgId msg_id, const char* data, uint64 length)
{
	LOG(INFO) << "Received unknown msg | " << msg_id;

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 发送handshake消息
 * 参  数: 
 * 返回值:
 * 修  改:
 *   奔?2013.10.26
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtPeerConnection::SendHandshakeMsg()
{
	TorrentSP torrent = this->torrent().lock();
	if (!torrent) return;

	LOG(INFO) << "Send BT handshake msg";

	MemBufferPool& mem_pool = torrent->session().mem_buf_pool();
	SendBuffer send_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

    char* buf = send_buf.buf;

	*buf = BT_HANDSHAKE_PROTOCOL_STR_LEN; //填充协议字符串长度
    buf += 1;

    std::memcpy(buf, BT_HANDSHAKE_PROTOCOL_STR, BT_HANDSHAKE_PROTOCOL_STR_LEN); //填充协议字符串
    buf += BT_HANDSHAKE_PROTOCOL_STR_LEN;

    //设置保留字段
    char reserved[8] = {0};
	//保留字段前两位设成0x6578,能保证对迅雷客户端的主动连接。 且迅雷，bitcomment的主动连接都有此字段
	reserved[0] |= 0x65;
	reserved[1] |= 0x78; 
    reserved[5] |= 0x10; //设置支持bt协议扩?    reserved[7] |= 0x05; //设置支持bt协议扩展
    std::memcpy(buf, reserved, 8); //填充保留字段
    buf += 8;

    std::memcpy(buf, torrent->info_hash()->raw_data().c_str(), 20); //填充20位的info-hash
    buf += 20;

    std::memcpy(buf, BtSession::GetLocalPeerId(), 20); //填充20位的peer id
    buf += 20;

    send_buf.len = buf - send_buf.buf; //设置发送数据长度

    socket_connection()->SendData(send_buf); //发送handshake消息
}

/*------------------------------------------------------------------------------
 * 描  述: 发送bitfield消息
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月26日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 发送keep-alive消息
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月26日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtPeerConnection::SendKeepAliveMsg()
{

}

/*------------------------------------------------------------------------------
 * 描  述: 发送choke/unchoke消息
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月26日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 发送interest/uninterest消息
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月26日
 *   作者 teck_zhou
 *   描述 创建
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
