/*#############################################################################
 * �ļ���   : piece_picker.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��12��01��
 * �ļ����� : PiecePicker����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "piece_picker.hpp"
#include "torrent.hpp"
#include "piece_priority_strategy.hpp"
#include "disk_io_job.hpp"
#include "rcs_util.hpp"
#include "io_oper_manager.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: [in] torrent ����torrent
 *         [in] piece_map torrent��piece map
 *         [in] strategy piece���ز���
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PiecePicker::PiecePicker(const TorrentSP& torrent
	, const boost::dynamic_bitset<>& piece_map, DownloadStrategy strategy)
	: torrent_(torrent)
	, verified_piece_num_(0)
	, piece_map_(piece_map)
{
	BC_ASSERT(strategy == SEQUENTIAL); // ��ǰֻʵ����һ�����ز���

	BC_ASSERT(torrent->GetPieceNum() == piece_map_.size());

	strategy_.reset(new PieceSequentialStrategy(torrent->GetPieceNum()));

	ConstructPieceInfo(torrent);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PiecePicker::~PiecePicker()
{

}

/*-----------------------------------------------------------------------------
 * ��  ��: ����piece��Ϣ
 * ��  ��: [in] torrent torrent
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::ConstructPieceInfo(const TorrentSP& torrent)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	uint32 piece_num = torrent->GetPieceNum();
	for (uint32 piece = 0; piece < piece_num; ++piece)
	{
		if (piece_map_[piece])
		{
			finished_pieces_.insert(
				std::make_pair(piece, FinishedPieceInfo(piece, true)));
			verified_piece_num_++;
			continue;
		}

		DownloadPieceInfo piece_info = ConstructDownloadPiece(torrent, piece);
		waiting_pieces_.insert(piece_info);
	}

	// �������piece���Ѿ�������ϣ�����������״̬
	if (verified_piece_num_ == piece_num)
	{
		torrent->SetSeed();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����һ��������piece��Ϣ
 * ��  ��: [in] piece ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PiecePicker::DownloadPieceInfo 
PiecePicker::ConstructDownloadPiece(const TorrentSP& torrent, uint32 piece)
{
	DownloadPieceInfo piece_info;

	piece_info.piece_index = piece;
	piece_info.finished_blocks = 0;
	piece_info.priority = strategy_->GetPriority(piece);

	uint32 block_num = torrent->GetBlockNum(piece);
	for (uint32 block = 0; block < block_num; ++block)
	{
		piece_info.blocks.push_back(BlockInfo(WAITING));
	}

	return piece_info;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������piece��ѡȡblock
 * ��  ��: [in] filter peer��piece map
 *         [in] num_blocks ѡȡblock����
 *         [out] blocks ѡȡ��block
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::PickBlocksFromDownloadingPieces(const boost::dynamic_bitset<>& filter, 
												  uint32 num_blocks, 
												  PieceBlockVec& blocks)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	if (downloading_pieces_.empty()) return;

	uint32 added_blocks = 0;
	PiecePriorityView& prio_view = downloading_pieces_.get<1>();
	PiecePriorityView::reverse_iterator iter = prio_view.rbegin();
	for ( ; iter != prio_view.rend(); ++iter)
	{
		if (!filter[iter->piece_index]) continue;

		uint32 block_num = iter->blocks.size();
		for (uint32 block = 0; block < block_num; ++block)
		{
			if (iter->blocks[block].block_state != WAITING) continue;

			// ���ǵ�Ч�ʣ�����ʹ��ǿת�����뱣֤�����޸�������
			DownloadPieceInfo& pi = const_cast<DownloadPieceInfo&>(*iter);

			LOG(INFO) << "####### Pick block | " << pi.piece_index << ":" << block;

			blocks.push_back(PieceBlock(pi.piece_index, block));
			pi.blocks[block].block_state = DOWNLOADING;

			if (++added_blocks >= num_blocks) return;
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ӵ�����piece��ѡȡblock
 * ��  ��: [in] filter peer��piece map
 *         [in] num_blocks ѡȡblock����
 *         [out] blocks ѡȡ��block
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::PickBlocksFromWaitingPieces(const boost::dynamic_bitset<>& filter, 
											  uint32 num_blocks, 
											  PieceBlockVec& blocks)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	if (waiting_pieces_.empty()) return;

	uint32 added_blocks = 0;
	PriorityPieceSet::iterator iter = waiting_pieces_.begin();
	for ( ; iter != waiting_pieces_.end(); )
	{
		if (!filter[iter->piece_index])
		{
			++iter;
			continue;
		}

		DownloadPieceInfo& pi = const_cast<DownloadPieceInfo&>(*iter);

		uint32 block_num = pi.blocks.size();
		for (uint32 block = 0; block < block_num; ++block)
		{
			LOG(INFO) << "####### ++Pick block | " << pi.piece_index << ":" << block;

			blocks.push_back(PieceBlock(pi.piece_index, block));
			pi.blocks[block].block_state = DOWNLOADING;

			if (++added_blocks >= num_blocks) break;
		}

		// ������piece��Ϊ��������piece
		downloading_pieces_.insert(DownloadingPieceSet::value_type(pi));
		iter = waiting_pieces_.erase(iter);

		if (added_blocks >= num_blocks) break;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ѡȡ��Ҫ���ص�block
 * ��  ��: [in] filter peer��piece map
 *         [in] num_blocks ѡȡblock����
 *         [out] interesting_blocks ѡȡ��block
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::PickBlocks(const boost::dynamic_bitset<>& filter, 
	                         uint32 num_blocks, 
							 PieceBlockVec& blocks)
{
	// ����ѡ���������ص�piece
	PickBlocksFromDownloadingPieces(filter, num_blocks, blocks);
	BC_ASSERT(blocks.size() <= num_blocks);

	if (blocks.size() >= num_blocks) return;

	// �����������������ӵȴ�������ѡ��
	uint32 need_num = num_blocks - blocks.size();
	PickBlocksFromWaitingPieces(filter, need_num, blocks);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ѡȡ��Ҫ���ص�piece���ֲ�ʽ����ʱʹ�á�
 *         ������ѡȡ�����piece�����ֻ�ܴӴ�����������ѡȡ��
 * ��  ��: [in] filter peer��piece map
 *         [in] num_blocks ѡȡblock����
 *         [out] pieces ѡȡ��piece
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::PickPieces(const boost::dynamic_bitset<>& filter, 
	                         uint32 num_pieces, 
							 PieceVec& pieces)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	if (waiting_pieces_.empty()) return;

	uint32 added_pieces = 0;

	PriorityPieceSet::iterator iter = waiting_pieces_.begin();
	for ( ; iter != waiting_pieces_.end(); )
	{
		DownloadPieceInfo pi = *iter;
		uint32 block_num = pi.blocks.size();
		for (uint32 block = 0; block < block_num; ++block)
		{
			pi.blocks[block].block_state = DOWNLOADING;
		}

		LOG(INFO) << "Pick piece : " << pi.piece_index;
		pieces.push_back(pi.piece_index);

		// ������piece��Ϊ��������piece
		downloading_pieces_.insert(DownloadingPieceSet::value_type(pi));
		iter = waiting_pieces_.erase(iter);

		if (++added_pieces >= num_pieces) break;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ˢ��piece�����ȼ�
 * ��  ��: [in] piece ����
 *         [out] priority �����õ����ȼ�
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::UpdatePiecePriority(uint32 piece, uint32 priority)
{
	LOG(WARNING) << "Update piece " << piece << " priority to " << priority;

	boost::mutex::scoped_lock lock(piece_mutex_);

	// �ȿ���Ҫ���µ�piece�Ƿ���������������

	PieceIndexView& index_view = downloading_pieces_.get<0>(); 
	auto view_iter = index_view.find(piece);
	if (view_iter != index_view.end()) // �ҵ���ˢ�����ȼ�
	{
		if (view_iter->priority == priority) return;

		DownloadPieceInfo piece_info = *view_iter;
		piece_info.priority = priority;

		// ��֤���ȼ��ߵ�piece���Ա���������
		index_view.erase(view_iter);
		index_view.insert(DownloadingPieceSet::value_type(piece_info));

		LOG(INFO) << "Update piece priority in downloading container successfully | " << piece;

		return;
	}

	// Ȼ���ٲ鿴����������

	PriorityPieceSet::iterator iter = waiting_pieces_.begin();
	for ( ; iter != waiting_pieces_.end(); ++iter)
	{
		if (iter->piece_index != piece) continue;

		if (iter->priority == priority) return;

		DownloadPieceInfo piece_info = *iter;
		piece_info.priority = priority;

		waiting_pieces_.erase(iter); // ������ζ�Ҫ�ӵȴ�����ɾ��

		if (ShouldPieceStartDownloading(priority))
		{
			downloading_pieces_.insert(DownloadingPieceSet::value_type(piece_info));
		}
		else
		{
			waiting_pieces_.insert(piece_info);
		}

		LOG(INFO) << "Update piece priority in waiting container successfully | " << piece;
		break;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������ȼ����������ض��е���С���ȼ����ߣ���˵��piece��Ҫ���뵽
 *         �����ض���������
 * ��  ��: [in] priority ���ȼ�
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��03��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PiecePicker::ShouldPieceStartDownloading(uint32 priority)
{
	PiecePriorityView& prio_view = downloading_pieces_.get<1>();
	PiecePriorityView::reverse_iterator iter = prio_view.rbegin();
	if (iter == prio_view.rend()) return false;

	if (priority > iter->priority)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����piece�����ȼ�Ϊ��ߣ�ʹָ��piece������������
 * ��  ��: [in] piece ��Ƭ
 * ����ֵ: ��/�� 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::SetPieceHighestPriority(uint32 piece)
{
	UpdatePiecePriority(piece, 7);
}

/*-----------------------------------------------------------------------------
 * ��  ��: piece�Ƿ�������
 * ��  ��: [in] piece ��Ƭ
 * ����ֵ: ��/�� 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PiecePicker::IsPieceFinished(uint32 piece)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	auto iter = finished_pieces_.find(piece);
	if (iter != finished_pieces_.end() && iter->second.verified) 
	{
		return true;
	}

	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: block�Ƿ�������
 * ��  ��: [in] block block
 * ����ֵ: ��/�� 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PiecePicker::IsBlockFinished(const PieceBlock& block)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	uint32 piece = block.piece_index;

	// �Ȳ���������piece
	if (finished_pieces_.find(piece) != finished_pieces_.end()) return true;

	// �ٲ����������ض���

	PieceIndexView& index_view = downloading_pieces_.get<0>(); 
	auto iter = index_view.find(piece);
	if (iter != index_view.end()
		&& (iter->blocks[block.block_index].block_state == FINISHED))
	{
		return true;
	}

	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���block����ʧ��
 * ��  ��: [in] block block
 * ����ֵ: ��/�� 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::MarkBlockFailed(const PieceBlock& block)
{
	LOG(WARNING) << "Mark block failed | " << block;

	boost::mutex::scoped_lock lock(piece_mutex_);

	PieceIndexView& index_view = downloading_pieces_.get<0>(); 
	auto iter = index_view.find(block.piece_index);

	BC_ASSERT(iter != index_view.end());
	BC_ASSERT(static_cast<uint32>(block.block_index) < iter->blocks.size());
	BC_ASSERT((*iter).blocks[block.block_index].block_state == DOWNLOADING);

	DownloadPieceInfo& pi = const_cast<DownloadPieceInfo&>(*iter);
	pi.blocks[block.block_index].block_state = WAITING;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���block�������
 * ��  ��: [in] block block
 * ����ֵ: ��/�� 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::MarkBlockFinished(const PieceBlock& block)
{
	LOG(INFO) << "Mark block finished | " << block;

	boost::mutex::scoped_lock lock(piece_mutex_);

	PieceIndexView& index_view = downloading_pieces_.get<0>(); 
	auto iter = index_view.find(block.piece_index);

	BC_ASSERT(iter != index_view.end());
	BC_ASSERT(static_cast<uint32>(block.block_index) < iter->blocks.size());
	BC_ASSERT(iter->blocks[block.block_index].block_state == DOWNLOADING);

	DownloadPieceInfo& pi = const_cast<DownloadPieceInfo&>(*iter);

	BC_ASSERT(pi.blocks[block.block_index].block_state == DOWNLOADING);

	pi.blocks[block.block_index].block_state = FINISHED;
	pi.finished_blocks++;

	if (iter->finished_blocks < iter->blocks.size()) return; // piece��δ������

	BC_ASSERT(iter->finished_blocks == iter->blocks.size());

	// piece������ɣ��Ƶ��������
	finished_pieces_.insert(
		std::make_pair(block.piece_index, FinishedPieceInfo(block.piece_index)));
	downloading_pieces_.erase(iter);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// piece���������ҪУ����hash
	torrent->io_oper_manager()->AsyncVerifyPieceInfoHash(
		boost::bind(&PiecePicker::OnVerifyPiece, this, _1, _2), block.piece_index);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���pieceʧ��
 * ��  ��: [in] piece ��Ƭ
 * ����ֵ: ��/�� 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::MarkPieceFailed(uint32 piece)
{
	LOG(INFO) << "Mark piece failed | " << piece;

	boost::mutex::scoped_lock lock(piece_mutex_);

	PieceIndexView& index_view = downloading_pieces_.get<0>(); 
	auto iter = index_view.find(piece);

	BC_ASSERT(iter != index_view.end());

	downloading_pieces_.erase(iter);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DownloadPieceInfo piece_info = ConstructDownloadPiece(torrent, piece);

	if (ShouldPieceStartDownloading(piece_info.priority))
	{
		downloading_pieces_.insert(DownloadingPieceSet::value_type(piece_info));
		LOG(INFO) << "Insert piece to downloading container | " << piece;
	}
	else
	{
		waiting_pieces_.insert(piece_info);
		LOG(INFO) << "Insert piece to waiting container | " << piece;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: piece hashУ�鴦��
 * ��  ��: [in] ok �Ƿ�ɹ�
 *         [in] job ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::OnVerifyPiece(bool ok, const DiskIoJobSP& job)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	VerifyPieceInfoHashJobSP verify_piece_job = SP_CAST(VerifyPieceInfoHashJob, job);
	uint32 piece = verify_piece_job->piece_index;

	if (!ok) // pieceУ��ʧ��
	{
		LOG(WARNING) << "Fail to verify piece hash | " << piece;
		FailToVerifyPieceProc(piece);
		return;
	}

	{
		boost::mutex::scoped_lock lock(piece_mutex_);

		auto iter = finished_pieces_.find(piece);
		BC_ASSERT(iter != finished_pieces_.end());

		iter->second.verified = true;

		BC_ASSERT(!piece_map_[piece]);
		piece_map_[piece] = true;

		verified_piece_num_++;
	}

	torrent->HavePiece(piece); // piece������ɣ�֪ͨhave piece

	// ����piece������ɣ����ÿ�ʼ����
	if (verified_piece_num_ == torrent->GetPieceNum())
	{
		torrent->SetSeed(); 
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: piece hashУ��ʧ�ܴ���
 * ��  ��: [in] piece ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PiecePicker::FailToVerifyPieceProc(uint32 piece)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	auto iter = finished_pieces_.find(piece);
	BC_ASSERT(iter != finished_pieces_.end());

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// ���¹���һ�������ص�piece
	DownloadPieceInfo piece_info = ConstructDownloadPiece(torrent, piece);

	waiting_pieces_.insert(piece_info);
	finished_pieces_.erase(iter);
}

}