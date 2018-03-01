/*#############################################################################
 * 文件名   : piece_picker.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年12月01日
 * 文件描述 : PiecePicker声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描  述: 构造函数
 * 参  数: [in] torrent 所属torrent
 *         [in] piece_map torrent的piece map
 *         [in] strategy piece下载策略
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
PiecePicker::PiecePicker(const TorrentSP& torrent
	, const boost::dynamic_bitset<>& piece_map, DownloadStrategy strategy)
	: torrent_(torrent)
	, verified_piece_num_(0)
	, piece_map_(piece_map)
{
	BC_ASSERT(strategy == SEQUENTIAL); // 当前只实现了一种下载策略

	BC_ASSERT(torrent->GetPieceNum() == piece_map_.size());

	strategy_.reset(new PieceSequentialStrategy(torrent->GetPieceNum()));

	ConstructPieceInfo(torrent);
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
PiecePicker::~PiecePicker()
{

}

/*-----------------------------------------------------------------------------
 * 描  述: 构建piece信息
 * 参  数: [in] torrent torrent
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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

	// 如果所有piece都已经下载完毕，则设置做种状态
	if (verified_piece_num_ == piece_num)
	{
		torrent->SetSeed();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 构建一个待下载piece信息
 * 参  数: [in] piece 索引
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 从正在下载piece中选取block
 * 参  数: [in] filter peer的piece map
 *         [in] num_blocks 选取block数量
 *         [out] blocks 选取的block
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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

			// 考虑到效率，这里使用强转，必须保证不会修改索引。
			DownloadPieceInfo& pi = const_cast<DownloadPieceInfo&>(*iter);

			LOG(INFO) << "####### Pick block | " << pi.piece_index << ":" << block;

			blocks.push_back(PieceBlock(pi.piece_index, block));
			pi.blocks[block].block_state = DOWNLOADING;

			if (++added_blocks >= num_blocks) return;
		}
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 从待下载piece中选取block
 * 参  数: [in] filter peer的piece map
 *         [in] num_blocks 选取block数量
 *         [out] blocks 选取的block
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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

		// 待下载piece变为正在下载piece
		downloading_pieces_.insert(DownloadingPieceSet::value_type(pi));
		iter = waiting_pieces_.erase(iter);

		if (added_blocks >= num_blocks) break;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 选取需要下载的block
 * 参  数: [in] filter peer的piece map
 *         [in] num_blocks 选取block数量
 *         [out] interesting_blocks 选取的block
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PiecePicker::PickBlocks(const boost::dynamic_bitset<>& filter, 
	                         uint32 num_blocks, 
							 PieceBlockVec& blocks)
{
	// 优先选择正在下载的piece
	PickBlocksFromDownloadingPieces(filter, num_blocks, blocks);
	BC_ASSERT(blocks.size() <= num_blocks);

	if (blocks.size() >= num_blocks) return;

	// 如果还不够，则继续从等待容器中选择
	uint32 need_num = num_blocks - blocks.size();
	PickBlocksFromWaitingPieces(filter, need_num, blocks);
}

/*-----------------------------------------------------------------------------
 * 描  述: 选取需要下载的piece，分布式下载时使用。
 *         由于是选取整块的piece，因此只能从待下载容器中选取。
 * 参  数: [in] filter peer的piece map
 *         [in] num_blocks 选取block数量
 *         [out] pieces 选取的piece
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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

		// 待下载piece变为正在下载piece
		downloading_pieces_.insert(DownloadingPieceSet::value_type(pi));
		iter = waiting_pieces_.erase(iter);

		if (++added_pieces >= num_pieces) break;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 刷新piece的优先级
 * 参  数: [in] piece 索引
 *         [out] priority 欲设置的优先级
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PiecePicker::UpdatePiecePriority(uint32 piece, uint32 priority)
{
	LOG(WARNING) << "Update piece " << piece << " priority to " << priority;

	boost::mutex::scoped_lock lock(piece_mutex_);

	// 先看下要更新的piece是否在正在下载容器

	PieceIndexView& index_view = downloading_pieces_.get<0>(); 
	auto view_iter = index_view.find(piece);
	if (view_iter != index_view.end()) // 找到则刷新优先级
	{
		if (view_iter->priority == priority) return;

		DownloadPieceInfo piece_info = *view_iter;
		piece_info.priority = priority;

		// 保证优先级高的piece可以被立即下载
		index_view.erase(view_iter);
		index_view.insert(DownloadingPieceSet::value_type(piece_info));

		LOG(INFO) << "Update piece priority in downloading container successfully | " << piece;

		return;
	}

	// 然后再查看待下载容器

	PriorityPieceSet::iterator iter = waiting_pieces_.begin();
	for ( ; iter != waiting_pieces_.end(); ++iter)
	{
		if (iter->piece_index != piece) continue;

		if (iter->priority == priority) return;

		DownloadPieceInfo piece_info = *iter;
		piece_info.priority = priority;

		waiting_pieces_.erase(iter); // 无论如何都要从等待队列删除

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
 * 描  述: 如果传入优先级比正在下载队列的最小优先级还高，则说明piece需要插入到
 *         正下载队列中来。
 * 参  数: [in] priority 优先级
 * 返回值: 
 * 修  改:
 *   时间 2013年12月03日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 设置piece的优先级为最高，使指定piece可以优先下载
 * 参  数: [in] piece 分片
 * 返回值: 是/否 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PiecePicker::SetPieceHighestPriority(uint32 piece)
{
	UpdatePiecePriority(piece, 7);
}

/*-----------------------------------------------------------------------------
 * 描  述: piece是否下载完
 * 参  数: [in] piece 分片
 * 返回值: 是/否 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: block是否下载完
 * 参  数: [in] block block
 * 返回值: 是/否 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PiecePicker::IsBlockFinished(const PieceBlock& block)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	uint32 piece = block.piece_index;

	// 先查找下载完piece
	if (finished_pieces_.find(piece) != finished_pieces_.end()) return true;

	// 再查找正在下载队列

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
 * 描  述: 标记block下载失败
 * 参  数: [in] block block
 * 返回值: 是/否 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 标记block下载完成
 * 参  数: [in] block block
 * 返回值: 是/否 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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

	if (iter->finished_blocks < iter->blocks.size()) return; // piece还未下载完

	BC_ASSERT(iter->finished_blocks == iter->blocks.size());

	// piece下载完成，移到完成容器
	finished_pieces_.insert(
		std::make_pair(block.piece_index, FinishedPieceInfo(block.piece_index)));
	downloading_pieces_.erase(iter);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// piece下载完后需要校验下hash
	torrent->io_oper_manager()->AsyncVerifyPieceInfoHash(
		boost::bind(&PiecePicker::OnVerifyPiece, this, _1, _2), block.piece_index);
}

/*-----------------------------------------------------------------------------
 * 描  述: 标记piece失败
 * 参  数: [in] piece 分片
 * 返回值: 是/否 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: piece hash校验处理
 * 参  数: [in] ok 是否成功
 *         [in] job 参数
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PiecePicker::OnVerifyPiece(bool ok, const DiskIoJobSP& job)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	VerifyPieceInfoHashJobSP verify_piece_job = SP_CAST(VerifyPieceInfoHashJob, job);
	uint32 piece = verify_piece_job->piece_index;

	if (!ok) // piece校验失败
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

	torrent->HavePiece(piece); // piece下载完成，通知have piece

	// 所有piece下载完成，设置开始做种
	if (verified_piece_num_ == torrent->GetPieceNum())
	{
		torrent->SetSeed(); 
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: piece hash校验失败处理
 * 参  数: [in] piece 索引
 * 返回值: 
 * 修  改:
 *   时间 2013年12月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PiecePicker::FailToVerifyPieceProc(uint32 piece)
{
	boost::mutex::scoped_lock lock(piece_mutex_);

	auto iter = finished_pieces_.find(piece);
	BC_ASSERT(iter != finished_pieces_.end());

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// 重新构造一个待下载的piece
	DownloadPieceInfo piece_info = ConstructDownloadPiece(torrent, piece);

	waiting_pieces_.insert(piece_info);
	finished_pieces_.erase(iter);
}

}