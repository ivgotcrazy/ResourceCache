/*#############################################################################
 * 文件名   : peer_connection.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : PeerConnection实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "peer_connection.hpp"
#include "p2p_fsm.hpp"
#include "info_hash.hpp"
#include "mem_buffer_pool.hpp"
#include "bc_assert.hpp"
#include "torrent.hpp"
#include "io_oper_manager.hpp"
#include "piece_picker.hpp"
#include "peer_request.hpp"
#include "session.hpp"
#include "socket_connection.hpp"
#include "policy.hpp"
#include "rcs_util.hpp"
#include "rcs_config_parser.hpp"
#include "bc_util.hpp"
#include "distri_download_msg_recognizer.hpp"
#include "distri_download_mgr.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
PeerConnection::PeerConnection(Session& s, const SocketConnectionSP& conn, 
	PeerType peer_type, uint64 download_bandwidth_limit, uint64 upload_bandwidth_limit)
    : session_(s)
    , socket_conn_(conn)
	, peer_type_(peer_type)
	, peer_conn_type_(PEER_CONN_COMMON)
	, alive_time_(0)
	, download_bandwidth_limit_(download_bandwidth_limit)
	, upload_bandwidth_limit_(upload_bandwidth_limit)
	, started_(false)
	, alive_time_without_traffic_(0)	
	, max_alive_time_without_traffic_(600)
{
	ConnectionType conn_type = socket_conn_->connection_type();

	// 对于被动连接来说，连接创建时的peer类型只能识别为inner peer或者outer peer
	// 此时还无法确认此连接到底是不是proxy，当然被动连接不可能是sponsor，所以
	// 被动连接在连接创建时只能是common，因此直接使用默认值即可，不用处理
	if ((conn_type == CONN_ACTIVE)
		&& (peer_type_ == PEER_INNER_PROXY || peer_type_ == PEER_OUTER_PROXY))
	{
		peer_conn_type_ = PEER_CONN_SPONSOR;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
PeerConnection::~PeerConnection()
{
	LOG(INFO) << ">>> Destructing peer connection | " << connection_id();

	Stop();

	socket_conn_->Close();
}

/*-----------------------------------------------------------------------------
 * 描  述: 公共初始化处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::Init()
{
	traffic_controller_.reset(new TrafficController(this, 
		download_bandwidth_limit_, upload_bandwidth_limit_));

	distri_download_mgr_.reset(new DistriDownloadMgr(this));

	fsm_.reset(new P2PFsm(this, traffic_controller_.get()));

	// 设置报文接收处理函数
	PeerConnectionWP weak_conn = shared_from_this();
	socket_conn_->set_recv_handler(boost::bind(
		&PeerConnection::OnReceiveData, this, weak_conn, _1, _2, _3));

	// 注册分布式下载协议消息识别器
	MsgRecognizerSP recognizer(new DistriDownloadMsgRecognizer());
	pkt_recombiner_.RegisterMsgRecognizer(recognizer, boost::bind(
		&DistriDownloadMgr::ProcDistriDownloadMsg, distri_download_mgr_.get(), _1, _2));

	// 注册协议消息识别器
	std::vector<MsgRecognizerSP> recognizers = CreateMsgRecognizers();
	BC_ASSERT(!recognizers.empty());
	for (MsgRecognizerSP& recognizer : recognizers)
	{
		pkt_recombiner_.RegisterMsgRecognizer(recognizer, 
			boost::bind(&PeerConnection::ProcProtocolMsg, this, _1, _2));
	}

	GET_RCS_CONFIG_INT("global.peerconnection.max-alive-time-without-traffic", max_alive_time_without_traffic_);

	Initialize(); // 调用协议模块初始化
}

/*-----------------------------------------------------------------------------
 * 描  述: 报文接收处理
 * 参  数: error 错误码
 *         buf 报文内容
 *         bytes_transferred 报文长度
 * 返回值:
 * 修  改:
 *   时间 2013年10月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::OnReceiveData(const PeerConnectionWP& weak_conn, 
								   ConnectionError error, 
								   const char* buf, 
								   uint64 bytes_transferred)
{
	PeerConnectionSP conn = weak_conn.lock();
	if (!conn) return;

	if (error != CONN_ERR_SUCCESS)
	{
		ProcessRecvError(error);
		return;
	}

	alive_time_without_traffic_ = 0;

	if (conn->connection_id().protocol == CONN_PROTOCOL_TCP)
	{
		pkt_recombiner_.AppendData(buf, bytes_transferred);
	}
	else
	{	
		//如果是udp协议不考虑分包，组包，粘包
		pkt_recombiner_.AppendUdpData(buf, bytes_transferred);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 协议报文处理
 * 参  数: [in] buf 报文
 *         [in] len 报文长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月18日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ProcProtocolMsg(const char* buf, uint64 len)
{
    MessageEntry fsm_msg(buf, len);
	fsm_->ProcessMsg(FSM_MSG_PKT, static_cast<void*>(&fsm_msg));
}

/*-----------------------------------------------------------------------------
 * 描  述: 运行连接
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PeerConnection::Start()
{
	if (!fsm_->StartFsm())
	{
		LOG(ERROR) << "Fail to start FSM";
		return false;
	}

	started_ = true;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 停止连接
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PeerConnection::Stop()
{
	started_ = false;

	fsm_->ProcessMsg(FSM_MSG_CLOSE, 0);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 秒定时器处理函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::OnTick()
{
	if (!started_) return;

	traffic_controller_->OnTick();

	distri_download_mgr_->OnTick();

	fsm_->ProcessMsg(FSM_MSG_TICK, 0);

	alive_time_++; //计时

	alive_time_without_traffic_++;  //未通信时间计时

	SecondTick(); // 调用协议模块处理

}

/*-----------------------------------------------------------------------------
 * 描  述: 设置torrent
 * 参  数: [in] torrent 所属torrent
 * 返回值:
 * 修  改:
 *   时间 2013年12月03日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::AttachTorrent(const TorrentSP& torrent) 
{ 
	torrent_ = torrent;

	if (torrent->is_ready() && piece_map_.empty())
	{
		InitBitfield(torrent->GetPieceNum());
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 连接是由Session创建的，创建时还无法解析报文，因此也就无法确认连接所
 *         属的Torrent，在Torrent创建完成后，要将连接绑定到所属的Torrent
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::AttachToTorrent(const InfoHashSP& info_hash)
{
	BC_ASSERT(!torrent_.lock());

	// 首先从Session中查找Torrent
	TorrentSP torrent = session_.FindTorrent(info_hash);
	if (!torrent)
	{
		torrent = session_.NewTorrent(info_hash);
		if (!torrent)
		{
			LOG(ERROR) << "Fail to new torrent | " << info_hash;
			return;
		}
	}

	// 对于被动连接，创建并关联torrent后，连接就交给torrent来管理
	torrent->policy()->IncommingPeerConnecton(shared_from_this());
	session_.RemovePeerConnection(connection_id());

	AttachTorrent(torrent); // torrent创建完成
}

/*-----------------------------------------------------------------------------
 * 描  述: 连接错误处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ProcessRecvError(ConnectionError error)
{
	socket_conn_->Close();

	fsm_->ProcessMsg(FSM_MSG_CLOSE, 0);
}

/*-----------------------------------------------------------------------------
 * 描  述: Torrent获取了Metadata通知所有PeerConnection
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::RetrievedMetadata()
{
	fsm_->ProcessMsg(FSM_MSG_GET_MEATADATA, 0);
}

/*-----------------------------------------------------------------------------
 * 描  述: 握手完成后需要从握手状态转移到传输状态
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::HandshakeComplete()
{
	fsm_->ProcessMsg(FSM_MSG_HANDSHAKED, 0);
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到数据块的处理
 * 参  数: [in] request 数据请求，由piece消息转换而来
 *         [in] data 请求对应的数据
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedDataResponse(const PeerRequest& request, const char* data)
{
	BC_ASSERT(data);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;
	
	torrent->io_oper_manager()->AsyncWrite(request, const_cast<char*>(data),
		boost::bind(&PeerConnection::OnWriteData, shared_from_this(), _1, _2));
}

/*-----------------------------------------------------------------------------
 * 描  述: block写入磁盘后的回调处理
 * 参  数: [in] ret 写入结果
 *         [in] job 写数据Job
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::OnWriteData(bool ret, const DiskIoJobSP& job)
{
	WriteDataJobSP write_job = SP_CAST(WriteDataJob, job);

	// 数据写入到缓存后通知TrafficController 
	traffic_controller_->ReceivedDataResponse(ToPeerRequest(*write_job));

	if (ret)
	{
		LOG(INFO) << "Success to write data to disk | " << *write_job;
	}
	else
	{
		LOG(ERROR) << "Fail to write data to disk | " << *write_job;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送数据请求
 * 参  数: [in] request 数据请求
 * 返回?
 * 修  改:
 *   时间 2013年09月05日
 *   作?teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::SendDataRequest(const PeerRequest& request)
{
	LOG(INFO) << "Send data request msg | " << request;

	SendBuffer send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);
	
	WriteDataRequestMsg(request, send_buf.buf, send_buf.len);

	socket_conn_->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送数据请求响应
 * 参  数: [in] request peer的数据请求
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::SendDataResponse(const PeerRequest& request)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	LOG(INFO) << "Send data response msg | " << request;

	// 申请数据空间，当前规定peer一次获取的数据不能超过一个block
	char* buf = session_.mem_buf_pool().AllocDataBuffer(DATA_BUF_BLOCK);

	torrent->io_oper_manager()->AsyncRead(request, buf,                           
		boost::bind(&PeerConnection::OnReadData, shared_from_this(), _1, _2));
}

/*-----------------------------------------------------------------------------
 * 描  述: 从磁盘读取数据后的回调处理
 * 参  数: [in] ret 结果
 *         [in] job 读数据Job
 * 返回值:
 * 修  改:
 *   时间 2013年09月16日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::OnReadData(bool ret, const DiskIoJobSP& job)
{
	ReadDataJobSP read_job = SP_CAST(ReadDataJob, job);

	if (ret)
	{
		// 申请发送缓存
		SendBuffer send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_DATA);
		
		// 数据响应发送完后，需要通知TrafficController，用以控制上传速率
		send_buf.send_buffer_handler = boost::bind(&TrafficController::OnSendDataResponse, 
			traffic_controller_.get(), _1, ToPeerRequest(*read_job));

		// 调用协议模块构造响应消息
		WriteDataResponseMsg(read_job, send_buf.buf, send_buf.len);

		// 发送消息
		socket_conn_->SendData(send_buf);

		LOG(INFO) << "Send response to peer | " << *(socket_conn_.get());
	}
	else
	{
		LOG(ERROR) << "Fail to read data from disk | " << *read_job;
	}

	// 释放申请的内存
	session_.mem_buf_pool().FreeDataBuffer(DATA_BUF_BLOCK, read_job->buf);
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到对端bitfiled请求后更新piece位图
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedBitfieldMsg(const boost::dynamic_bitset<>& piece_map)
{	
	piece_map_ = piece_map;

	distri_download_mgr_->OnBitfield(piece_map);
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到Choke/Unchoke消息
 * 参  数: [in] choke 是否阻塞
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedChokeMsg(bool choke)
{
	if (choke)
	{
		fsm_->ProcessMsg(FSM_MSG_CHOKED, 0);
	}
	else
	{
		fsm_->ProcessMsg(FSM_MSG_UNCHOKED, 0);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到Interested/Uninterested消息
 * 参  数: [in] interest 是否感兴趣
 * 返回值:
 * 修  改:
 *   时间 2013年09月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedInterestMsg(bool interest)
{
	if (interest)
	{
		fsm_->ProcessMsg(FSM_MSG_INTEREST, 0);
	}
	else
	{
		fsm_->ProcessMsg(FSM_MSG_NONINTEREST, 0);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取到InfoHash
 * 参  数: [in] info_hash 资源InfoHash
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ObtainedInfoHash(const InfoHashSP& info_hash)
{
	// 主动连接也会收到通知，无需处理
	TorrentSP torrent = torrent_.lock();
	if (torrent) return;

	AttachToTorrent(info_hash);
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到数据请求
 * 参  数: [in] request 请求
 * 返回值:
 * 修  改:
 *   时间 2013年09月16日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedDataRequest(const PeerRequest& request)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// 请求的piece是否已经下载
	if (!torrent->piece_picker()->IsPieceFinished(request.piece))
	{
		LOG(WARNING) << "Request unfinished piece(" << request.piece << ")";
		return;
	}

	// 请求的长度是否合法
	if (request.start + request.length > torrent->GetCommonPieceSize())
	{
		LOG(WARNING) << "Invalid data request | " << request;
		return;
	}

	// 交由TrafficController去处理
	traffic_controller_->ReceivedDataRequest(request);
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到have-piece消息处理
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013.09.30
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedHaveMsg(uint64 piece)
{
	BC_ASSERT(piece < piece_map_.size());

	piece_map_[piece] = true;

	distri_download_mgr_->OnHavePiece(static_cast<uint32>(piece));
}

/*-----------------------------------------------------------------------------
 * 描  述: 通知其它peer自己已经获取了一个piece
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013.09.30
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PeerConnection::HavePiece(uint64 piece)
{
    SendHaveMsg(piece);
}

/*------------------------------------------------------------------------------
 * 描  述: 初始化对方peer的bitfield数据
 *         这个在对方没有发送bitfield有用(此时对方peer没有下载任何piece)
 * 参  数: [in] pieces pieces总数目,也是bitfield的大小
 * 返回值:
 * 修  改:
 *   时间 2013.10.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PeerConnection::InitBitfield(uint64 pieces)
{
    if (piece_map_.empty())
    {
        piece_map_.resize(pieces);
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 判断连接是否关闭
 * 参  数: 
 * 返回值: 是否关闭
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool PeerConnection::IsClosed()
{
	return fsm_->IsClosed();
}

/*-----------------------------------------------------------------------------
 * 描  述: 判断peeer_conneciton是否处于非活跃状态
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年1月21日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PeerConnection::IsInactive() const
{
	bool over_time = (alive_time_without_traffic_ >= max_alive_time_without_traffic_ ? true : false);
	
	if (over_time)
	{
		LOG(INFO) << "Peer Connection InActive With " << max_alive_time_without_traffic_ << " s";
	}
	
	return alive_time_without_traffic_ >= max_alive_time_without_traffic_;
}

/*------------------------------------------------------------------------------
 * 描  述: 发送have-piece消息
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月26日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
void PeerConnection::SendHaveMsg(uint64 piece_index)
{
	LOG(INFO) << "Send BT have msg | piece: " << piece_index;

	MemBufferPool& mem_pool = session_.mem_buf_pool();
	SendBuffer have_piece_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

	WriteHaveMsg(piece_index, have_piece_buf.buf, have_piece_buf.len);

	socket_connection()->SendData(have_piece_buf);
}

}
