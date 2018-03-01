/*#############################################################################
 * �ļ���   : traffic_controller.cpp
 * ������   : teck_zhou
 * ����ʱ�� : 2013��10��13��
 * �ļ����� : TrafficController��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "depend.hpp"
#include "traffic_controller.hpp"
#include "peer_connection.hpp"
#include "bc_assert.hpp"
#include "torrent.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

const int64 kWaitDataResponseTimeout = 3; 	// ������������󣬵ȴ���Ӧ��ʱʱ��
const uint64 kTryDownloadFragmentTimes = 3; // ������Ӧ��ʱ�����³������ش���
const uint64 kSpeedQueueMaxSize = 10;

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��24��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��24��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �����ϴ��ٶ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���������ٶ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��ʱ������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��24��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TrafficController::OnTick()
{
	DownloadTimeoutProc();

	UpdateDownloadSpeed();

	UpdateUploadSpeed();
}

/*-----------------------------------------------------------------------------
 * ��  ��: fragment����ʧ�ܴ�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��24��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TrafficController::FailToDownloadFragment()
{
	PendingFragment frag = download_frag_queue_.front();

	LOG(WARNING) << "Fail to download fragment | " << frag.block;

	// һ��fragment����ʧ�ܣ�������block����Ҫ��������
	while (!download_frag_queue_.empty()
		   && download_frag_queue_.front().block == frag.block)
	{
		download_frag_queue_.pop();
	}

	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return;

	// ֪ͨPiecePicker block����ʧ��
	torrent->piece_picker()->MarkBlockFailed(frag.block);

	download_block_failed = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���س�ʱ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���س�ʱ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��25��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TrafficController::DownloadTimeoutProc()
{
	boost::mutex::scoped_lock lock(download_mutex_);

	// û����������
	if (download_frag_queue_.empty()) return;

	PendingFragment& fragment = download_frag_queue_.front();

	// ����������ͣ
	if (!fragment.downloading) return;

	int64 duration = GetDurationSeconds(fragment.request_time, TimeNow());
	BC_ASSERT(duration >= 0);

	if (duration >= kWaitDataResponseTimeout)
	{
		if (fragment.tried_times >= kTryDownloadFragmentTimes)
		{
			FailToDownloadFragment();
		}
		else // �����������أ������·�����������
		{
			ReDownloadFragment(fragment);
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��block�ֽ�Ϊ���ص�fragment
 * ��  ��: [in] pb block��Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��25��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��ȡ�����ٶ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetDownloadSpeed() const 
{ 
	auto queue_size = download_speed_queue_.size();
	return (queue_size == 0) ? 0 : (total_download_size_in_queue_ / queue_size); 
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�ϴ��ٶ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetUploadSpeed() const 
{ 
	auto queue_size = upload_speed_queue_.size();
	return (queue_size == 0) ? 0 : (total_upload_size_in_queue_ / queue_size); 
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ��ǰ���ڵ������ٶ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetCurrentSecondDownloadSpeed()
{
	if (download_speed_queue_.empty()) return 0;

	return download_speed_queue_.back();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ��ǰ���ڵ��ϴ��ٶ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 TrafficController::GetCurrentSecondUploadSpeed()
{
	if (upload_speed_queue_.empty()) return 0;

	return upload_speed_queue_.back();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����ٶ��Ƿ��Ѿ��ﵽ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool TrafficController::IsDownloadSpeedLimited()
{
	return (GetDownloadSpeed() >= download_speed_limit_) 
		|| (GetCurrentSecondDownloadSpeed() >= current_download_speed_limit_);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ϴ��ٶ��Ƿ��Ѿ��ﵽ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool TrafficController::IsUploadSpeedLimited()
{
	return (GetUploadSpeed() >= upload_speed_limit_) 
		|| (GetCurrentSecondUploadSpeed() >= current_upload_speed_limit_);
}

/*-----------------------------------------------------------------------------
 * ��  ��: blockд����̺�Ļص�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TrafficController::TrySendDataRequest()
{
	LOG(INFO) << "Try Send Data Request ";

	if (download_block_failed) return;
	
	// ���Ϊproxy���ӣ����ܹ���sponsor��������
	if (peer_conn_->peer_conn_type() == PEER_CONN_PROXY) return;

	// ���ش�������
	if (IsDownloadSpeedLimited()) return;

	boost::mutex::scoped_lock lock(download_mutex_);

	if (download_frag_queue_.empty())
	{
		// ���block����Ϊ�գ�����Ҫ��������block
		if (download_frag_queue_.empty())
		{
			if (!ApplyDownloadBlock()) return;
			BC_ASSERT(!download_frag_queue_.empty());
		}
	}

	// ǰ�������δ���������ȷ���
	PendingFragment& fragment = download_frag_queue_.front();
	if (fragment.downloading) return;

	fragment.downloading = true;
	fragment.request_time = TimeNow();
	fragment.tried_times++;

	SendFragmentDataRequest(fragment);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����δ����block
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��24��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool TrafficController::ApplyDownloadBlock()
{
	TorrentSP torrent = peer_conn_->torrent().lock();
	if (!torrent) return false;

	std::vector<PieceBlock> blocks;

	// ��PiecePicker��������block
	torrent->piece_picker()->PickBlocks(peer_conn_->piece_map_, APPLY_DOWNLOAD_BLOCK_NUM, blocks);
	if (blocks.empty()) return false;

	// ��block�ֽ�Ϊfragment
	for (PieceBlock& piece_block : blocks)
	{
		ResolveBlockToDownloadFragments(piece_block);
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����peer��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TrafficController::TryProcPeerRequest()
{
	// �ϴ���������
	if (IsUploadSpeedLimited()) return;

	boost::mutex::scoped_lock lock(peer_request_mutex_);

	if (peer_request_queue_.empty()) return;

	// ǰ��������Ƿ��Ѿ�������
	PendingRequest& request = peer_request_queue_.front();
	if (request.processing) return;

	peer_conn_->SendDataResponse(request.request);
	
	request.processing = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������
 * ��  ��: request ��������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��13��
 *   ���� teck_zhou
 *   ���� ����
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
	
	// ���������û���������
	TryProcPeerRequest();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƚ�
 * ��  ��: [in] fragment fragment
 *         [in] request ��������
 * ����?
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ����������Ӧ����
 * ��  ��: success �ɹ���־
 *         request ��Ӧ��Ӧ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��13��
 *   ���� teck_zhou
 *   ���� ����
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

		// ����ϵͳ��һ��fragment���������ȥ������һ��fragment��������Ӧ��
		// ����ƥ����������������ڳ�ʱ����block�Ѿ��������أ�Ȼ�����յ���Ӧ
		// ����ʲô������˴�ֱ�Ӷ�������
		if (!IsEqual(fragment, request))
		{
			LOG(ERROR) << "Received unexpected data response | waiting: " 
				       << fragment.block << " incomming: " << request;
			return;
		}

		// ��¼ǰһ��block
		PieceBlock pre_block = fragment.block;
		
		// �����������block
		download_frag_queue_.pop();

		// �����ٶ�ͳ��
		{
			boost::mutex::scoped_lock lock(download_speed_mutex_);
			download_speed_queue_.back() += request.length;
			total_download_size_ += request.length;
			total_download_size_in_queue_ += request.length;
		}

		TorrentSP torrent = peer_conn_->torrent().lock();
		BC_ASSERT(torrent);
		
		// ���û�д����ص�fragment�ˣ���˵��block�Ѿ��������
		if (download_frag_queue_.empty())
		{
			LOG(INFO) << "####### Block finished | " << pre_block; 
			torrent->piece_picker()->MarkBlockFinished(pre_block);
		}
		else
		{
			// ������Ҫ�Ƚ�������fragment�뽫Ҫ����fragment��block����
			// �Ƿ�һ�£������һ�£�˵����һ��block��һ���µ�block
			PendingFragment& next_fragment = download_frag_queue_.front();
			if (pre_block != next_fragment.block)
			{
				LOG(INFO) << "####### Block finished | " << pre_block;
				torrent->piece_picker()->MarkBlockFinished(pre_block);
			}
		}
	}

	// ����������Ӧ����
	TrySendDataRequest();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��27��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ֹͣ�ϴ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��27��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TrafficController::StopUpload()
{
	boost::mutex::scoped_lock lock(peer_request_mutex_);

	RequestQueue empty_queue;

	peer_request_queue_.swap(empty_queue);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������Ӧ����
 * ��  ��: [in] error ������
 *         [in] request ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��04��
 *   ���� teck_zhou
 *   ���� ����
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

	// �ϴ��ٶ�ͳ��
	{
		boost::mutex::scoped_lock lock(upload_speed_mutex_);
		upload_speed_queue_.back() += request.length;
		total_upload_size_ += request.length;
		total_upload_size_in_queue_ += request.length;
	}
	
	// ���������û�����
	TryProcPeerRequest();
}

}
