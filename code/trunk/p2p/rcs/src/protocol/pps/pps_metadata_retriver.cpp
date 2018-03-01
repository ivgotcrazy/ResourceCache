/*#############################################################################
 * �ļ���   : pps_metadata_retriver.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��31��
 * �ļ����� : Metadata��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_metadata_retriver.hpp"
#include "torrent.hpp"
#include "pps_torrent.hpp"
#include "pps_peer_connection.hpp"
#include "bc_util.hpp"
#include "policy.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

const uint64 kPpsMetadataRequestTimeout = 10;
static const uint32 kPpsMetadataPieceSize = 1 * 1024; // Э��涨

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��17��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsMetadataRetriver::PpsMetadataRetriver(const TorrentSP& torrent) 
	: torrent_(torrent), retriving_(false)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ȡmetadta
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsMetadataRetriver::Start()
{
	BC_ASSERT(!retriving_);

	retriving_ = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������piece����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsMetadataRetriver::ConstructPendingPiece()
{
	uint64 total_size = metadata_size_;
	uint64 piece_index = 0;
	while (total_size > 0)
	{
		PendingMetadataPiece metadata_piece;
		if (total_size <= kPpsMetadataPieceSize)
		{
			metadata_piece.piece = piece_index;
			metadata_piece.size = total_size;
			download_piece_queue_.push(metadata_piece);
			break;
		}
		else
		{
			metadata_piece.piece = piece_index;
			metadata_piece.size = kPpsMetadataPieceSize;
			download_piece_queue_.push(metadata_piece);
		}

		piece_index++;
		total_size -= kPpsMetadataPieceSize;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ӻ�ȡѡ����
 * ��  ��: [in] conn ����
 * ����ֵ: true  ����������
 *         false ������������
 * ��  ��:
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsMetadataRetriver::ConnectionSelectFunc(const PeerConnectionSP& conn)
{
	PpsPeerConnectionSP pps_conn = SP_CAST(PpsPeerConnection, conn);
	if (!pps_conn) return false;

	return pps_conn->support_metadata_extend() && !pps_conn->retrive_metadata_failed();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ֧��metadta��չЭ��Ŀ�������
 * ��  ��: 
 * ����ֵ:     
 * ��  ��:
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsMetadataRetriver::GetCandidateConnection()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	std::vector<PeerConnectionSP> conns;
	conns = torrent->policy()->SelectConnection(
		boost::bind(&PpsMetadataRetriver::ConnectionSelectFunc, this, _1));

	if (conns.empty()) return false;

	PpsPeerConnectionSP pps_conn;
	for (PeerConnectionSP& conn : conns)
	{
		pps_conn = SP_CAST(PpsPeerConnection, conn);
		candidate_conns_.push(pps_conn);
	}

	// ��һ�λ�ȡ���������ӣ���Ҫ��������piece
	if (download_piece_queue_.empty())
	{
		//metadata_size_  �ڹ��캯����ʱ������
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
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsMetadataRetriver::TrySendPieceRequest()
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
	PpsPeerConnectionSP pps_conn = candidate_conns_.front();
	pps_conn->SendMetadataRequestMsg(pending_piece.piece, pending_piece.size);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsMetadataRetriver::TimeoutProc()
{
	if (download_piece_queue_.empty()) return;

	PendingMetadataPiece& pending_piece = download_piece_queue_.front();

	if (!pending_piece.downloading) return;

	int64 seconds = GetDurationSeconds(pending_piece.request_time, TimeNow());

	BC_ASSERT(seconds >= 0);

	if (static_cast<uint64>(seconds) < kPpsMetadataRequestTimeout) return;

	// ��ʱ�����·���piece����
	pending_piece.downloading = false;

	// ��ʱ��������Ϊ�����ã���Ǵ����Ӳ�ɾ��
	PpsPeerConnectionSP& pps_conn = candidate_conns_.front();

	LOG(INFO) << "Request metadata time out | " << pps_conn->connection_id();
	pps_conn->retrive_metadata_failed(true);
	
	candidate_conns_.pop();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����Ϣ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsMetadataRetriver::TickProc()
{
	if (!retriving_) return;

	LOG(INFO) << "Metadata Retriver Tick";

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
 *   ʱ�� 2013.12.26
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsMetadataRetriver::ReceivedMetadataDataMsg(PpsMetadataDataMsg& msg)
{
	if (!retriving_) return;

	LOG(WARNING) << "PPS Metadata Retrive Data Msg "; 

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
		BC_ASSERT(metadata_cache_.size() == metadata_size_);

		TorrentSP torrent = torrent_.lock();
		if (!torrent) return;

		PpsTorrentSP bt_torrent = SP_CAST(PpsTorrent, torrent);
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
}
