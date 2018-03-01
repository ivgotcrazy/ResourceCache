/*#############################################################################
 * 文件名   : distri_download_mgr.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月20日
 * 文件描述 : DistriDownloadMgr实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <boost/dynamic_bitset.hpp>

#include "distri_download_mgr.hpp"
#include "rcs_config.hpp"
#include "rcs_util.hpp"
#include "peer_connection.hpp"
#include "rcs_config_parser.hpp"
#include "bc_assert.hpp"
#include "torrent.hpp"
#include "policy.hpp"
#include "mem_buffer_pool.hpp"
#include "session.hpp"
#include "bc_io.hpp"

namespace BroadCache
{

static const uint32 kMaxWaitProxyDownloadTime = 30;

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 略
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DistriDownloadMgr::DistriDownloadMgr(PeerConnection* peer_conn) 
	: peer_conn_(peer_conn)
	, support_cache_proxy_(false)
	, received_assign_piece_msg_(false)
	, after_assigned_piece_time_(0)
{
	BC_ASSERT(peer_conn_);

	GET_RCS_CONFIG_BOOL("bt.policy.support-cache-proxy", support_cache_proxy_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DistriDownloadMgr::~DistriDownloadMgr()
{
	// 未下载完的piece需要通知PiecePicker
	TorrentSP torrent = peer_conn_->torrent().lock();
	if (torrent)
	{
		for (uint32 piece : assigned_pieces_)
		{
			torrent->piece_picker()->MarkPieceFailed(piece);
		}
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理缓存的piece
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::ProcCachedPieces()
{
	if (peer_conn_->peer_conn_type() != PEER_CONN_PROXY) return;

	if (cached_pieces_.empty()) return;

	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	if (!torrent->is_ready()) return;

	for (uint32 piece : cached_pieces_)
	{
		torrent->piece_picker()->SetPieceHighestPriority(piece);
	}

	cached_pieces_.clear();
}

/*-----------------------------------------------------------------------------
 * 描  述: proxy下载超时处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::TimeoutProc()
{
	if (peer_conn_->peer_conn_type() != PEER_CONN_SPONSOR) return;

	if (assigned_pieces_.empty()) return;

	after_assigned_piece_time_++;

	if (after_assigned_piece_time_ >= kMaxWaitProxyDownloadTime)
	{
		LOG(WARNING) << "Proxy connection timed out, close it";
		peer_conn_->Stop();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnTick()
{
	if (peer_conn_->peer_conn_type() == PEER_CONN_COMMON) return;

	TryAssignPieces();

	ProcCachedPieces();

	TimeoutProc();
}

/*-----------------------------------------------------------------------------
 * 描  述: 分布式下载报文消息处理
 * 参  数: [in] buf 报文
 *         [in] len 报文长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::ProcDistriDownloadMsg(const char* buf, uint64 len)
{
	DistriDownloadMsgType msg_type = GetDistriDownloadMsgType(buf, len);
	
	uint32 piece = ParseDistriDownloadMsg(buf, len);

	switch (msg_type)
	{
	case DISTRI_DOWNLOAD_MSG_ASSIGN_PIECE:
		OnAssignPieceMsg(piece);
		break;

	case DISTRI_DOWNLOAD_MSG_ACCEPT_PIECE:
		OnAcceptPieceMsg(piece);
		break;

	case DISTRI_DOWNLOAD_MSG_REJECT_PIECE:
		OnRejectPieceMsg(piece);
		break;

	default:
		BC_ASSERT(false);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 对于分布式下载连接，握手成功即可指派远端代理下载piece
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::TryAssignPieces()
{
	if (peer_conn_->peer_conn_type() != PEER_CONN_SPONSOR) return;

	if (!assigned_pieces_.empty()) return;

	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	if (!torrent->is_ready()) return;

	AssignPieces(torrent);
}

/*-----------------------------------------------------------------------------
 * 描  述: 看下指派的piece是不是已经下载完毕
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnHavePiece(uint32 piece)
{
	LOG(INFO) << "Have msg proc for distributed download";

	if (peer_conn_->peer_conn_type() != PEER_CONN_SPONSOR) return;

	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	std::set<uint32>::iterator iter = assigned_pieces_.find(piece);
	if (iter == assigned_pieces_.end()) return;

	assigned_pieces_.erase(iter);

	BC_ASSERT(torrent->piece_picker());
	torrent->piece_picker()->MarkPieceFailed(piece);

	TryAssignPieces();
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到bitfield消息也需要处理下:
 *         1) 发送assign-piece的时候，还未收到bitfiled，可能对端已经有此piece。
 * 参  数: [in] piece_map 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnBitfield(const boost::dynamic_bitset<>& piece_map)
{
	LOG(INFO) << "Bitfield msg proc for distributed download";

	if (peer_conn_->peer_conn_type() != PEER_CONN_SPONSOR) return;

	if (assigned_pieces_.empty()) return;

	std::set<uint32>::iterator iter = assigned_pieces_.begin();
	for ( ; iter != assigned_pieces_.end(); )
	{
		if (piece_map[*iter])
		{
			TorrentSP torrent = peer_conn_->torrent().lock();
			if (!torrent) return;

			BC_ASSERT(torrent->piece_picker());
			torrent->piece_picker()->MarkPieceFailed(*iter);

			iter = assigned_pieces_.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	TryAssignPieces();
}

/*-----------------------------------------------------------------------------
 * 描  述: 看下指派的piece是不是已经下载完毕
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::AssignPieces(const TorrentSP& torrent)
{
	boost::dynamic_bitset<> piece_map = peer_conn_->piece_map();
	if (piece_map.empty()) return;

	PiecePicker* piece_picker = torrent->piece_picker();
	if (!piece_picker) return;

	std::vector<uint32> proxy_pieces;
	piece_picker->PickPieces(piece_map, 1, proxy_pieces); // 取一个piece

	if (proxy_pieces.empty()) return;

	for (int piece : proxy_pieces)
	{
		SendAssignPieceMsg(piece);

		assigned_pieces_.insert(piece);
	}

	after_assigned_piece_time_ = 0; // 重新开始计算超时
}

/*-----------------------------------------------------------------------------
 * 描  述: 分派piece消息处理
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnAssignPieceMsg(uint32 piece)
{
	LOG(INFO) << "Received assgin-piece msg | " << piece;

	if (!support_cache_proxy_)
	{
		SendRejectPieceMsg(piece);
		return;
	}

	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	// 第一次收到assign-piece消息，需要尝试设置torrent为代理
	if (!received_assign_piece_msg_)
	{
		// 收到assgin-piece的连接都是代理连接
		peer_conn_->SetPeerConnType(PEER_CONN_PROXY);

		// 收到assign-piece连接肯定为被动连接，如果torrent中仅此一个被动连接
		// 则说明所属torrent应该为proxy，当然，这里的逻辑不是非常严密，对于BT
		// 协议，其他peer可以根据DHT协议找到RCS从而建立被动连接，不过由于
		// assign-piece消息是紧接着handshake发送，从时间上来看，当前问题不大
		if (torrent->policy()->GetPassivePeerConnNum() == 1)
		{
			torrent->SetProxy();
		}

		received_assign_piece_msg_ = true;
	}

	// 如果PiecePicker已经创建，则可以将分派的piece直接传给PiecePicker
	PiecePicker* picker = torrent->piece_picker();
	if (torrent->is_ready() && picker)
	{
		picker->SetPieceHighestPriority(piece);
	}
	else // 否则就只能先将分派的piece缓存起来，等后面PiecePicker创建后再处理
	{
		cached_pieces_.push_back(piece);
	}

	// 回复接受此piece
	SendAcceptPieceMsg(piece);
}

/*-----------------------------------------------------------------------------
 * 描  述: 接受分派piece消息处理
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnAcceptPieceMsg(uint32 piece)
{
	BC_ASSERT(peer_conn_->peer_conn_type() == PEER_CONN_SPONSOR);

	LOG(INFO) << "Received accept-piece msg | " << piece;
}

/*-----------------------------------------------------------------------------
 * 描  述: 拒绝分派piece消息处理
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnRejectPieceMsg(uint32 piece)
{
	BC_ASSERT(peer_conn_->peer_conn_type() == PEER_CONN_SPONSOR);

	LOG(INFO) << "Received reject-piece msg | " << piece;

	peer_conn_->Stop();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取分布式下载协议消息具体类型
 * 参  数: [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DistriDownloadMsgType DistriDownloadMgr::GetDistriDownloadMsgType(const char* buf, uint64 len)
{
	// <magic-number><length><type><data>

	BC_ASSERT(len >= 4 + 2 + 1);

	char* tmp = const_cast<char*>(buf) + 4 + 2;

	return (DistriDownloadMsgType)(IO::read_uint8(tmp));
}

/*-----------------------------------------------------------------------------
 * 描  述: 由于分布式下载协议消息payload都是piece，因此公用一个消息解析函数
 * 参  数: [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint32 DistriDownloadMgr::ParseDistriDownloadMsg(const char* buf, uint64 len)
{
	// <magic-number><length><type><data>
	const char* tmp = buf + 4 + 2 + 1;

	return IO::read_uint32(tmp);
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造assign-piece消息
 * 参  数: [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::ConstructAssignPieceMsg(uint32 piece, char* buf, uint64* len)
{
	// <magic-number><length><type><data>   

	IO::write_uint32(DISTRIBUTED_DOWNLOAD_MAGIC_NUMBER, buf);

	IO::write_uint16(5, buf);

	IO::write_uint8(DISTRI_DOWNLOAD_MSG_ASSIGN_PIECE, buf);

	IO::write_uint32(piece, buf);

	*len = 11;
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造accept-piece消息
 * 参  数: [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::ConstructAcceptPieceMsg(uint32 piece, char* buf, uint64* len)
{
	// <magic-number><length><type><data>  

	IO::write_uint32(DISTRIBUTED_DOWNLOAD_MAGIC_NUMBER, buf);

	IO::write_uint16(5, buf);

	IO::write_uint8(DISTRI_DOWNLOAD_MSG_ACCEPT_PIECE, buf);

	IO::write_uint32(piece, buf);

	*len = 11;
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造reject-piece消息
 * 参  数: [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::ConstructRejectPieceMsg(uint32 piece, char* buf, uint64* len)
{
	// <magic-number><length><type><data>  

	IO::write_uint32(DISTRIBUTED_DOWNLOAD_MAGIC_NUMBER, buf);

	IO::write_uint16(5, buf);

	IO::write_uint8(DISTRI_DOWNLOAD_MSG_REJECT_PIECE, buf);

	IO::write_uint32(piece, buf);

	*len = 11;
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送报文消息
 * 参  数: [in] piece piece索引
 *         [in] constructor 消息构造器
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::SendMsgImpl(uint32 piece, MsgConstructor constructor)
{
	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	MemBufferPool& mem_pool = torrent->session().mem_buf_pool();

	SendBuffer send_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

	constructor(piece, send_buf.buf, &send_buf.len);

	peer_conn_->socket_connection()->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送assgin-piece消息
 * 参  数: [in] piece piece索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::SendAssignPieceMsg(uint32 piece)
{
	SendMsgImpl(piece, boost::bind(
		&DistriDownloadMgr::ConstructAssignPieceMsg, this, _1, _2, _3));

	LOG(INFO) << "Send assgin-piece msg | " << piece;
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送accept-piece消息
 * 参  数: [in] piece piece索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::SendAcceptPieceMsg(uint32 piece)
{
	SendMsgImpl(piece, boost::bind(
		&DistriDownloadMgr::ConstructAcceptPieceMsg, this, _1, _2, _3));

	LOG(INFO) << "Send accept-piece msg | " << piece;
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送reject-piece消息
 * 参  数: [in] piece piece索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::SendRejectPieceMsg(uint32 piece)
{
	SendMsgImpl(piece, boost::bind(
		&DistriDownloadMgr::ConstructRejectPieceMsg, this, _1, _2, _3));

	LOG(INFO) << "Send reject-piece msg | " << piece;
}


}
