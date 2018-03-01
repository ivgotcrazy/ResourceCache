/*#############################################################################
 * 文件名   : bt_metadata_retriver.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年09月16日
 * 文件描述 : Metadata类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bt_metadata_retriver.hpp"
#include "torrent.hpp"
#include "bt_torrent.hpp"
#include "bt_peer_connection.hpp"
#include "bc_util.hpp"
#include "policy.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

const uint64 kMetadataRequestTimeout = 10;
const uint64 kMetadataPieceSize = 1024 * 16;

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年09月17日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
BtMetadataRetriver::BtMetadataRetriver(const TorrentSP& torrent) 
	: torrent_(torrent), retriving_(false)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动获取metadta
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::Start()
{
	BC_ASSERT(!retriving_);

	retriving_ = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建下载piece队列
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::ConstructPendingPiece()
{
	uint64 total_size = metadata_total_size_;
	uint64 piece_index = 0;
	while (total_size > 0)
	{
		PendingMetadataPiece metadata_piece;
		if (total_size <= kMetadataPieceSize)
		{
			metadata_piece.piece = piece_index;
			metadata_piece.size = total_size;
			download_piece_queue_.push(metadata_piece);
			break;
		}
		else
		{
			metadata_piece.piece = piece_index;
			metadata_piece.size = kMetadataPieceSize;
			download_piece_queue_.push(metadata_piece);
		}

		piece_index++;
		total_size -= kMetadataPieceSize;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 连接获取选择函数
 * 参  数: [in] conn 连接
 * 返回值: true  是所需连接
 *         false 不是所需连接
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtMetadataRetriver::ConnectionSelectFunc(const PeerConnectionSP& conn)
{
	BtPeerConnectionSP bt_conn = SP_CAST(BtPeerConnection, conn);
	if (!bt_conn) return false;

	return bt_conn->support_metadata_extend() && !bt_conn->retrive_metadata_failed();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取支持metadta扩展协议的可用连接
 * 参  数: 
 * 返回值: true  获取到连接
 *         false 没有获取到连接
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtMetadataRetriver::GetCandidateConnection()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	std::vector<PeerConnectionSP> conns;
	conns = torrent->policy()->SelectConnection(
		boost::bind(&BtMetadataRetriver::ConnectionSelectFunc, this, _1));

	if (conns.empty()) return false;

	BtPeerConnectionSP bt_conn;
	for (PeerConnectionSP& conn : conns)
	{
		bt_conn = SP_CAST(BtPeerConnection, conn);
		candidate_conns_.push(bt_conn);
	}

	// 第一次获取到可用连接，需要构造下载piece
	if (download_piece_queue_.empty())
	{
		metadata_total_size_ = bt_conn->metadata_extend_msg().metadata_size;
		ConstructPendingPiece();
		BC_ASSERT(!download_piece_queue_.empty());
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送请求
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::TrySendPieceRequest()
{
	BC_ASSERT(!download_piece_queue_.empty());

	PendingMetadataPiece& pending_piece = download_piece_queue_.front();
	
	// 如果请求发送后还没有收到响应则等待
	if (pending_piece.downloading) return;

	// 没有可用连接，那就等后面获取到连接后再处理吧
	if (candidate_conns_.empty()) return;

	pending_piece.downloading = true;
	pending_piece.request_time = TimeNow();
	
	// 永远使用队列首部的连接
	BtPeerConnectionSP bt_conn = candidate_conns_.front();
	bt_conn->SendMetadataRequestMsg(pending_piece.piece);
}

/*-----------------------------------------------------------------------------
 * 描  述: 超时处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::TimeoutProc()
{
	if (download_piece_queue_.empty()) return;

	PendingMetadataPiece& pending_piece = download_piece_queue_.front();

	if (!pending_piece.downloading) return;

	int64 seconds = GetDurationSeconds(pending_piece.request_time, TimeNow());

	BC_ASSERT(seconds >= 0);

	if (static_cast<uint64>(seconds) < kMetadataRequestTimeout) return;

	// 超时后，重新发送piece请求
	pending_piece.downloading = false;

	// 超时的连接认为不可用，标记此连接并删除
	BtPeerConnectionSP& bt_conn = candidate_conns_.front();

	LOG(INFO) << "Request metadata time out | " << bt_conn->connection_id();
	bt_conn->retrive_metadata_failed(true);
	
	candidate_conns_.pop();
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器消息处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::TickProc()
{
	if (!retriving_) return;

	boost::mutex::scoped_lock lock(download_mutex_);

	// 如果还没有连接可用，则先获取连接
	if (candidate_conns_.empty())
	{
		if (!GetCandidateConnection())
		{
			LOG(WARNING) << "Fail to get metadata candidate connection";
			return;
		}		
	}

	TimeoutProc();

	TrySendPieceRequest();
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析收到的消息
 * 参  数: [in] msg 消息
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::ReceivedMetadataDataMsg(BtMetadataDataMsg& msg)
{
	if (!retriving_) return;

	boost::mutex::scoped_lock lock(download_mutex_);

	BC_ASSERT(!download_piece_queue_.empty());
	PendingMetadataPiece& pending_piece = download_piece_queue_.front();

	if (pending_piece.piece != msg.piece)
	{
		LOG(WARNING) << "Received unexpected metadata piece | expect:" 
			         << pending_piece.piece << " come: " << msg.piece;
		return;
	}

	if (pending_piece.size != msg.len)
	{
		LOG(ERROR) << "Received invalid metadata piece | expect size:" 
			<< pending_piece.size << " come size: " << msg.len;
		return;
	}

	metadata_cache_.append(msg.buf, msg.len);

	download_piece_queue_.pop();

	// 下载完成
	if (download_piece_queue_.empty())
	{
		BC_ASSERT(metadata_cache_.size() == metadata_total_size_);

		TorrentSP torrent = torrent_.lock();
		if (!torrent) return;

		BtTorrentSP bt_torrent = SP_CAST(BtTorrent, torrent);
		bt_torrent->RetrivedMetadata(metadata_cache_);
		retriving_ = false;

		ConnectionQueue empty_queue;
		candidate_conns_.swap(empty_queue);
	}
	else
	{
		TrySendPieceRequest();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 收到metadta-reject消息处理
 * 参  数: [in] msg 消息数据
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::ReceivedMetadataRejectMsg(BtMetadataRejectMsg& msg)
{
	if (!retriving_) return;

	boost::mutex::scoped_lock lock(download_mutex_);

	BC_ASSERT(!download_piece_queue_.empty());
	PendingMetadataPiece& pending_piece = download_piece_queue_.front();

	if (msg.piece != pending_piece.piece)
	{
		LOG(WARNING) << "Received invalid metadata reject msg | expect piece: " 
			         << pending_piece.piece << " come piece: " << msg.piece;
		return;
	}

	// 超时后，重新发送piece请求
	pending_piece.downloading = false;

	// 超时的连接认为不可用，不再重发
	candidate_conns_.pop();
}

}
