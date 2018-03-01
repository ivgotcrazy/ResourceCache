/*#############################################################################
 * 文件名   : traffic_controller.cpp
 * 创建人   : teck_zhou
 * 创建时间 : 2013年10月13日
 * 文件描述 : TrafficController类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "depend.hpp"
#include "traffic_controller.hpp"
#include "peer_connection.hpp"
#include "bc_assert.hpp"
#include "torrent.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

const int64 kWaitDataResponseTimeout = 3; 	// 发送数据请求后，等待响应超时时间
const uint64 kTryDownloadFragmentTimes = 3; // 数据响应超时后，重新尝试下载次数
const uint64 kSpeedQueueMaxSize = 10;

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
TrafficController::TrafficController(PeerConnection* peer_conn, 
	uint64 download_speed_limit, uint64 upload_speed_limit)
	: peer_conn_(peer_conn)
	, download_speed_limit_(download_speed_limit)
	, upload_speed_limit_(upload_speed_limit)
	, fragment_size_(DEFAULT_FRAGMENT_SIZE)
	, total_download_size_(0)
	, total_download_size_in_queue_(0)
	, total_upload_size_(0)
	, total_upload_size_in_queue_(0)
	, download_block_failed(false)
{
	BC_ASSERT(peer_conn_);

	current_download_speed_limit_ = 1.1 * download_speed_limit; 
	current_upload_speed_limit_   = 1.1 * upload_speed_limit;

	upload_speed_queue_.push(0);
	download_speed_queue_.push(0);
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::SendFragmentDataRequest(const PendingFragment& fragment)
{
	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	PeerRequest request;
	request.piece = fragment.block.piece_index;
	request.start = (fragment.block.block_index * torrent->GetCommonBlockSize())
		+ fragment.frag_index * fragment_size_;
	request.length = fragment.frag_len;

	LOG(INFO) << "Send data request | " << request;

	peer_conn_->SendDataRequest(request);
}

/*-----------------------------------------------------------------------------
 * 描  述: 更新上传速度
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::UpdateDownloadSpeed()
{
	boost::mutex::scoped_lock lock(download_speed_mutex_);

	if (!download_speed_queue_.empty()) 
	{
		total_download_size_in_queue_ -= download_speed_queue_.front();
	}

	if (download_speed_queue_.size() >= kSpeedQueueMaxSize)
	{
		download_speed_queue_.pop();
	}

	download_speed_queue_.push(0);
}

/*-----------------------------------------------------------------------------
 * 描  述: 更新下载速度
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::UpdateUploadSpeed()
{
	boost::mutex::scoped_lock lock(upload_speed_mutex_);

	if (!upload_speed_queue_.empty())
	{
		total_upload_size_in_queue_ -= upload_speed_queue_.front();
	}

	if (upload_speed_queue_.size() >= kSpeedQueueMaxSize)
	{
		upload_speed_queue_.pop();
	}

	upload_speed_queue_.push(0);
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::OnTick()
{
	DownloadTimeoutProc();

	UpdateDownloadSpeed();

	UpdateUploadSpeed();
}

/*-----------------------------------------------------------------------------
 * 描  述: fragment下载失败处理函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::FailToDownloadFragment()
{
	PendingFragment frag = download_frag_queue_.front();

	LOG(WARNING) << "Fail to download fragment | " << frag.block;

	// 一个fragment下载失败，则整个block都需要重新下载
	while (!download_frag_queue_.empty()
		   && download_frag_queue_.front().block == frag.block)
	{
		download_frag_queue_.pop();
	}

	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	// 通知PiecePicker block下载失败
	torrent->piece_picker()->MarkBlockFailed(frag.block);

	download_block_failed = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 下载超时处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::ReDownloadFragment(PendingFragment& fragment)
{
	BC_ASSERT(fragment.tried_times < kTryDownloadFragmentTimes);

	fragment.tried_times++;
	fragment.request_time = TimeNow();

	SendFragmentDataRequest(fragment);

	LOG(WARNING) << "Try to re-download framgment | " << fragment.block
				 << " | frag_index : " << fragment.frag_index
				 << " | block_offset : " << fragment.block_offset
				 << " | fragment len : " << fragment.frag_len
		         << " | try times: " << fragment.tried_times;
}

/*-----------------------------------------------------------------------------
 * 描  述: 下载超时处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月25日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::DownloadTimeoutProc()
{
	boost::mutex::scoped_lock lock(download_mutex_);

	// 没有下载任务
	if (download_frag_queue_.empty()) return;

	PendingFragment& fragment = download_frag_queue_.front();

	// 下载任务暂停
	if (!fragment.downloading) return;

	int64 duration = GetDurationSeconds(fragment.request_time, TimeNow());
	BC_ASSERT(duration >= 0);

	if (duration >= kWaitDataResponseTimeout)
	{
		if (fragment.tried_times >= kTryDownloadFragmentTimes)
		{
			FailToDownloadFragment();
		}
		else // 还允许尝试下载，则重新发送数据请求
		{
			ReDownloadFragment(fragment);
		}
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 将block分解为下载的fragment
 * 参  数: [in] pb block信息
 * 返回值:
 * 修  改:
 *   时间 2013年10月25日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::ResolveBlockToDownloadFragments(const PieceBlock& pb)
{
	PendingFragment fragment;
	fragment.block = pb;

	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	uint64 block_size = torrent->GetBlockSize(pb.piece_index, pb.block_index);
	uint64 fragment_index = 0;
	while (block_size > 0)
	{
		if (block_size > fragment_size_)
		{
			fragment.frag_len = fragment_size_;
			block_size -= fragment_size_;
		}
		else
		{
			fragment.frag_len = block_size;
			block_size = 0;
		}
		fragment.block_offset = fragment_index * fragment_size_;
		fragment.frag_index = fragment_index;

		download_frag_queue_.push(fragment);
		fragment_index++;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取下载速度
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetDownloadSpeed() const 
{ 
	auto queue_size = download_speed_queue_.size();
	return (queue_size == 0) ? 0 : (total_download_size_in_queue_ / queue_size); 
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取上传速度
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetUploadSpeed() const 
{ 
	auto queue_size = upload_speed_queue_.size();
	return (queue_size == 0) ? 0 : (total_upload_size_in_queue_ / queue_size); 
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取当前秒内的下载速度
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetCurrentSecondDownloadSpeed()
{
	if (download_speed_queue_.empty()) return 0;

	return download_speed_queue_.back();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取当前秒内的上传速度
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetCurrentSecondUploadSpeed()
{
	if (upload_speed_queue_.empty()) return 0;

	return upload_speed_queue_.back();
}

/*-----------------------------------------------------------------------------
 * 描  述: 下载速度是否已经达到上限
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool TrafficController::IsDownloadSpeedLimited()
{
	return (GetDownloadSpeed() >= download_speed_limit_) 
		|| (GetCurrentSecondDownloadSpeed() >= current_download_speed_limit_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 上传速度是否已经达到上限
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool TrafficController::IsUploadSpeedLimited()
{
	return (GetUploadSpeed() >= upload_speed_limit_) 
		|| (GetCurrentSecondUploadSpeed() >= current_upload_speed_limit_);
}

/*-----------------------------------------------------------------------------
 * 描  述: block写入磁盘后的回调处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::TrySendDataRequest()
{
	LOG(INFO) << "Try Send Data Request ";

	if (download_block_failed) return;
	
	// 如果为proxy连接，则不能够向sponsor发起请求
	if (peer_conn_->peer_conn_type() == PEER_CONN_PROXY) return;

	// 下载带宽限制
	if (IsDownloadSpeedLimited()) return;

	boost::mutex::scoped_lock lock(download_mutex_);

	if (download_frag_queue_.empty())
	{
		// 如果block队列为空，则需要申请下载block
		if (download_frag_queue_.empty())
		{
			if (!ApplyDownloadBlock()) return;
			BC_ASSERT(!download_frag_queue_.empty());
		}
	}

	// 前面的请求还未处理完则先返回
	PendingFragment& fragment = download_frag_queue_.front();
	if (fragment.downloading) return;

	fragment.downloading = true;
	fragment.request_time = TimeNow();
	fragment.tried_times++;

	SendFragmentDataRequest(fragment);
}

/*-----------------------------------------------------------------------------
 * 描  述: 申请未下载block
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool TrafficController::ApplyDownloadBlock()
{
	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return false;

	std::vector<PieceBlock> blocks;

	// 从PiecePicker申请下载block
	torrent->piece_picker()->PickBlocks(peer_conn_->piece_map_, APPLY_DOWNLOAD_BLOCK_NUM, blocks);
	if (blocks.empty()) return false;

	// 将block分解为fragment
	for (PieceBlock& piece_block : blocks)
	{
		ResolveBlockToDownloadFragments(piece_block);
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理peer数据请求
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::TryProcPeerRequest()
{
	// 上传带宽限制
	if (IsUploadSpeedLimited()) return;

	boost::mutex::scoped_lock lock(peer_request_mutex_);

	if (peer_request_queue_.empty()) return;

	// 前面的请求是否已经处理返回
	PendingRequest& request = peer_request_queue_.front();
	if (request.processing) return;

	peer_conn_->SendDataResponse(request.request);
	
	request.processing = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 新增数据请求
 * 参  数: request 数据请求
 * 返回值:
 * 修  改:
 *   时间 2013年10月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::ReceivedDataRequest(const PeerRequest& request)
{
	{
		boost::mutex::scoped_lock lock(peer_request_mutex_);

		PendingRequest p_request;
		p_request.request = request;
		p_request.processing = false;

		peer_request_queue_.push(p_request);
	}
	
	// 触发处理用户数据请求
	TryProcPeerRequest();
}

/*-----------------------------------------------------------------------------
 * 描  述: 比较
 * 参  数: [in] fragment fragment
 *         [in] request 数据请求
 * 祷刂?
 * 修  改:
 *   时间 2013年10月26日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool TrafficController::IsEqual(const PendingFragment& fragment, const PeerRequest& request)
{
	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return false;

	uint64 piece_offset = fragment.block.block_index * torrent->GetCommonBlockSize() 
						  + fragment.block_offset;

	return (fragment.block.piece_index == static_cast<int>(request.piece))
		&& (piece_offset == request.start)
		&& (fragment.frag_len == request.length);
}

/*-----------------------------------------------------------------------------
 * 描  述: 数据请求响应处理
 * 参  数: success 成功标志
 *         request 响应对应的请求
 * 返回值:
 * 修  改:
 *   时间 2013年10月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::ReceivedDataResponse(const PeerRequest& request)
{
	{
		boost::mutex::scoped_lock lock(download_mutex_);

		if (download_frag_queue_.empty())
		{
			LOG(WARNING) << "No request in download queue | " << request.piece;
			return;
		}

		PendingFragment& fragment = download_frag_queue_.front();
		BC_ASSERT(download_frag_queue_.front().downloading);

		// 由于系统是一个fragment下载完后再去下载下一个fragment，出现响应与
		// 请求不匹配情况，可能是由于超时，此block已经放弃下载，然后又收到响应
		// 不管什么情况，此处直接丢弃即可
		if (!IsEqual(fragment, request))
		{
			LOG(ERROR) << "Received unexpected data response | waiting: " 
				       << fragment.block << " incomming: " << request;
			return;
		}

		// 记录前一个block
		PieceBlock pre_block = fragment.block;
		
		// 弹出下载完的block
		download_frag_queue_.pop();

		// 下载速度统计
		{
			boost::mutex::scoped_lock lock(download_speed_mutex_);
			download_speed_queue_.back() += request.length;
			total_download_size_ += request.length;
			total_download_size_in_queue_ += request.length;
		}

		TorrentSP torrent = peer_conn_->torrent().lock();
		BC_ASSERT(torrent);
		
		// 如果没有待下载的fragment了，则说明block已经下载完毕
		if (download_frag_queue_.empty())
		{
			LOG(INFO) << "####### Block finished | " << pre_block; 
			torrent->piece_picker()->MarkBlockFinished(pre_block);
		}
		else
		{
			// 否则需要比较已下载fragment与将要下载fragment的block索引
			// 是否一致，如果不一致，说明下一个block是一个新的block
			PendingFragment& next_fragment = download_frag_queue_.front();
			if (pre_block != next_fragment.block)
			{
				LOG(INFO) << "####### Block finished | " << pre_block;
				torrent->piece_picker()->MarkBlockFinished(pre_block);
			}
		}
	}

	// 继续处理响应请求
	TrySendDataRequest();
}

/*-----------------------------------------------------------------------------
 * 描  述: 停止下载
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::StopDownload()
{
	boost::mutex::scoped_lock lock(download_mutex_);

	while (!download_frag_queue_.empty())
	{
		FailToDownloadFragment();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 停止上传
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::StopUpload()
{
	boost::mutex::scoped_lock lock(peer_request_mutex_);

	RequestQueue empty_queue;

	peer_request_queue_.swap(empty_queue);
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送数据响应处理
 * 参  数: [in] error 错误码
 *         [in] request 请求
 * 返回值:
 * 修  改:
 *   时间 2013年11月04日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void TrafficController::OnSendDataResponse(ConnectionError error, const PeerRequest& request)
{
	{
		boost::mutex::scoped_lock lock(peer_request_mutex_);

		if (peer_request_queue_.empty()) return;

		PendingRequest& pending_request = peer_request_queue_.front();
		BC_ASSERT(pending_request.request == request);

		peer_request_queue_.pop();
	}

	// 上传速度统计
	{
		boost::mutex::scoped_lock lock(upload_speed_mutex_);
		upload_speed_queue_.back() += request.length;
		total_upload_size_ += request.length;
		total_upload_size_in_queue_ += request.length;
	}
	
	// 继续处理用户请求
	TryProcPeerRequest();
}

}
