/*#############################################################################
 * �ļ���   : bt_metadata_retriver.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��09��16��
 * �ļ����� : Metadata��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��17��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtMetadataRetriver::BtMetadataRetriver(const TorrentSP& torrent) 
	: torrent_(torrent), retriving_(false)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ȡmetadta
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::Start()
{
	BC_ASSERT(!retriving_);

	retriving_ = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������piece����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���ӻ�ȡѡ����
 * ��  ��: [in] conn ����
 * ����ֵ: true  ����������
 *         false ������������
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtMetadataRetriver::ConnectionSelectFunc(const PeerConnectionSP& conn)
{
	BtPeerConnectionSP bt_conn = SP_CAST(BtPeerConnection, conn);
	if (!bt_conn) return false;

	return bt_conn->support_metadata_extend() && !bt_conn->retrive_metadata_failed();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ֧��metadta��չЭ��Ŀ�������
 * ��  ��: 
 * ����ֵ: true  ��ȡ������
 *         false û�л�ȡ������
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
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

	// ��һ�λ�ȡ���������ӣ���Ҫ��������piece
	if (download_piece_queue_.empty())
	{
		metadata_total_size_ = bt_conn->metadata_extend_msg().metadata_size;
		ConstructPendingPiece();
		BC_ASSERT(!download_piece_queue_.empty());
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::TrySendPieceRequest()
{
	BC_ASSERT(!download_piece_queue_.empty());

	PendingMetadataPiece& pending_piece = download_piece_queue_.front();
	
	// ��������ͺ�û���յ���Ӧ��ȴ�
	if (pending_piece.downloading) return;

	// û�п������ӣ��Ǿ͵Ⱥ����ȡ�����Ӻ��ٴ����
	if (candidate_conns_.empty()) return;

	pending_piece.downloading = true;
	pending_piece.request_time = TimeNow();
	
	// ��Զʹ�ö����ײ�������
	BtPeerConnectionSP bt_conn = candidate_conns_.front();
	bt_conn->SendMetadataRequestMsg(pending_piece.piece);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::TimeoutProc()
{
	if (download_piece_queue_.empty()) return;

	PendingMetadataPiece& pending_piece = download_piece_queue_.front();

	if (!pending_piece.downloading) return;

	int64 seconds = GetDurationSeconds(pending_piece.request_time, TimeNow());

	BC_ASSERT(seconds >= 0);

	if (static_cast<uint64>(seconds) < kMetadataRequestTimeout) return;

	// ��ʱ�����·���piece����
	pending_piece.downloading = false;

	// ��ʱ��������Ϊ�����ã���Ǵ����Ӳ�ɾ��
	BtPeerConnectionSP& bt_conn = candidate_conns_.front();

	LOG(INFO) << "Request metadata time out | " << bt_conn->connection_id();
	bt_conn->retrive_metadata_failed(true);
	
	candidate_conns_.pop();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����Ϣ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtMetadataRetriver::TickProc()
{
	if (!retriving_) return;

	boost::mutex::scoped_lock lock(download_mutex_);

	// �����û�����ӿ��ã����Ȼ�ȡ����
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
 * ��  ��: �����յ�����Ϣ
 * ��  ��: [in] msg ��Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
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

	// �������
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
 * ��  ��: �յ�metadta-reject��Ϣ����
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
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

	// ��ʱ�����·���piece����
	pending_piece.downloading = false;

	// ��ʱ��������Ϊ�����ã������ط�
	candidate_conns_.pop();
}

}
