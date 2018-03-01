/*#############################################################################
 * �ļ���   : distri_download_mgr.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��20��
 * �ļ����� : DistriDownloadMgrʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��: ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DistriDownloadMgr::~DistriDownloadMgr()
{
	// δ�������piece��Ҫ֪ͨPiecePicker
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
 * ��  ��: �������piece
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: proxy���س�ʱ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��ʱ������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnTick()
{
	if (peer_conn_->peer_conn_type() == PEER_CONN_COMMON) return;

	TryAssignPieces();

	ProcCachedPieces();

	TimeoutProc();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ֲ�ʽ���ر�����Ϣ����
 * ��  ��: [in] buf ����
 *         [in] len ���ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���ڷֲ�ʽ�������ӣ����ֳɹ�����ָ��Զ�˴�������piece
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ����ָ�ɵ�piece�ǲ����Ѿ��������
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �յ�bitfield��ϢҲ��Ҫ������:
 *         1) ����assign-piece��ʱ�򣬻�δ�յ�bitfiled�����ܶԶ��Ѿ��д�piece��
 * ��  ��: [in] piece_map ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ����ָ�ɵ�piece�ǲ����Ѿ��������
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::AssignPieces(const TorrentSP& torrent)
{
	boost::dynamic_bitset<> piece_map = peer_conn_->piece_map();
	if (piece_map.empty()) return;

	PiecePicker* piece_picker = torrent->piece_picker();
	if (!piece_picker) return;

	std::vector<uint32> proxy_pieces;
	piece_picker->PickPieces(piece_map, 1, proxy_pieces); // ȡһ��piece

	if (proxy_pieces.empty()) return;

	for (int piece : proxy_pieces)
	{
		SendAssignPieceMsg(piece);

		assigned_pieces_.insert(piece);
	}

	after_assigned_piece_time_ = 0; // ���¿�ʼ���㳬ʱ
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����piece��Ϣ����
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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

	// ��һ���յ�assign-piece��Ϣ����Ҫ��������torrentΪ����
	if (!received_assign_piece_msg_)
	{
		// �յ�assgin-piece�����Ӷ��Ǵ�������
		peer_conn_->SetPeerConnType(PEER_CONN_PROXY);

		// �յ�assign-piece���ӿ϶�Ϊ�������ӣ����torrent�н���һ����������
		// ��˵������torrentӦ��Ϊproxy����Ȼ��������߼����Ƿǳ����ܣ�����BT
		// Э�飬����peer���Ը���DHTЭ���ҵ�RCS�Ӷ������������ӣ���������
		// assign-piece��Ϣ�ǽ�����handshake���ͣ���ʱ������������ǰ���ⲻ��
		if (torrent->policy()->GetPassivePeerConnNum() == 1)
		{
			torrent->SetProxy();
		}

		received_assign_piece_msg_ = true;
	}

	// ���PiecePicker�Ѿ�����������Խ����ɵ�pieceֱ�Ӵ���PiecePicker
	PiecePicker* picker = torrent->piece_picker();
	if (torrent->is_ready() && picker)
	{
		picker->SetPieceHighestPriority(piece);
	}
	else // �����ֻ���Ƚ����ɵ�piece�����������Ⱥ���PiecePicker�������ٴ���
	{
		cached_pieces_.push_back(piece);
	}

	// �ظ����ܴ�piece
	SendAcceptPieceMsg(piece);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ܷ���piece��Ϣ����
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnAcceptPieceMsg(uint32 piece)
{
	BC_ASSERT(peer_conn_->peer_conn_type() == PEER_CONN_SPONSOR);

	LOG(INFO) << "Received accept-piece msg | " << piece;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ܾ�����piece��Ϣ����
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::OnRejectPieceMsg(uint32 piece)
{
	BC_ASSERT(peer_conn_->peer_conn_type() == PEER_CONN_SPONSOR);

	LOG(INFO) << "Received reject-piece msg | " << piece;

	peer_conn_->Stop();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�ֲ�ʽ����Э����Ϣ��������
 * ��  ��: [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DistriDownloadMsgType DistriDownloadMgr::GetDistriDownloadMsgType(const char* buf, uint64 len)
{
	// <magic-number><length><type><data>

	BC_ASSERT(len >= 4 + 2 + 1);

	char* tmp = const_cast<char*>(buf) + 4 + 2;

	return (DistriDownloadMsgType)(IO::read_uint8(tmp));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ڷֲ�ʽ����Э����Ϣpayload����piece����˹���һ����Ϣ��������
 * ��  ��: [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint32 DistriDownloadMgr::ParseDistriDownloadMsg(const char* buf, uint64 len)
{
	// <magic-number><length><type><data>
	const char* tmp = buf + 4 + 2 + 1;

	return IO::read_uint32(tmp);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����assign-piece��Ϣ
 * ��  ��: [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ����accept-piece��Ϣ
 * ��  ��: [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ����reject-piece��Ϣ
 * ��  ��: [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���ͱ�����Ϣ
 * ��  ��: [in] piece piece����
 *         [in] constructor ��Ϣ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ����assgin-piece��Ϣ
 * ��  ��: [in] piece piece����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::SendAssignPieceMsg(uint32 piece)
{
	SendMsgImpl(piece, boost::bind(
		&DistriDownloadMgr::ConstructAssignPieceMsg, this, _1, _2, _3));

	LOG(INFO) << "Send assgin-piece msg | " << piece;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����accept-piece��Ϣ
 * ��  ��: [in] piece piece����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::SendAcceptPieceMsg(uint32 piece)
{
	SendMsgImpl(piece, boost::bind(
		&DistriDownloadMgr::ConstructAcceptPieceMsg, this, _1, _2, _3));

	LOG(INFO) << "Send accept-piece msg | " << piece;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����reject-piece��Ϣ
 * ��  ��: [in] piece piece����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DistriDownloadMgr::SendRejectPieceMsg(uint32 piece)
{
	SendMsgImpl(piece, boost::bind(
		&DistriDownloadMgr::ConstructRejectPieceMsg, this, _1, _2, _3));

	LOG(INFO) << "Send reject-piece msg | " << piece;
}


}
