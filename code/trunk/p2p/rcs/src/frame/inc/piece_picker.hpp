/*#############################################################################
 * 文件名   : piece_picker.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年12月01日
 * 文件描述 : PiecePickerNew声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PIECE_PICKER
#define HEADER_PIECE_PICKER

#include <map>
#include <set>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/noncopyable.hpp>
#include "rcs_typedef.hpp"
#include "piece_structs.hpp"


namespace BroadCache
{

class PiecePriorityStrategy;

using namespace boost::multi_index;

/******************************************************************************
 * 描述: piece下载控制
 * 作者：teck_zhou
 * 时间：2013/12/01
 *****************************************************************************/
class PiecePicker : public boost::noncopyable
{
public:
	typedef std::vector<PieceBlock> PieceBlockVec;
	typedef std::vector<uint32> PieceVec;

	enum DownloadStrategy
	{
		SEQUENTIAL	// pick pieces in sequential order
	};

public:
	PiecePicker(const TorrentSP& torrent, 
		        const boost::dynamic_bitset<>& piece_map,
				DownloadStrategy strategy = SEQUENTIAL);
	~PiecePicker();

	void PickBlocks(const boost::dynamic_bitset<>& filter, 
		            uint32 num_blocks, 
					PieceBlockVec& blocks);

	void PickPieces(const boost::dynamic_bitset<>& filter, 
		            uint32 num_pieces, 
					PieceVec& pieces);

	void SetPieceHighestPriority(uint32 piece);

	bool IsPieceFinished(uint32 piece);
	bool IsBlockFinished(const PieceBlock& block); 

	void MarkBlockFailed(const PieceBlock& block);
	void MarkBlockFinished(const PieceBlock& block);
	void MarkPieceFailed(uint32 piece);

	boost::dynamic_bitset<> GetPieceMap() { return piece_map_; }

private:

	enum BlockState
	{
		WAITING,		// 等待下载
		DOWNLOADING,	// 正在下载
		FINISHED		// 下载完毕
	};

	struct BlockInfo
	{
		BlockInfo(BlockState state) : block_state(state) {}

		BlockState block_state;
	};

	struct DownloadPieceInfo
	{
		DownloadPieceInfo() 
			: piece_index(0xFFFFFFFF)
			, priority(0)
			, finished_blocks(0) {}
		DownloadPieceInfo(uint32 piece) 
			: piece_index(piece)
			, priority(0)
			, finished_blocks(0) {}

		uint32 piece_index;
		uint32 priority;
		uint32 finished_blocks;
		std::vector<BlockInfo> blocks;
	};

	struct FinishedPieceInfo
	{
		FinishedPieceInfo(uint32 piece, bool is_verified) 
			: piece_index(piece), verified(is_verified) {}
		FinishedPieceInfo(uint32 piece)
			: piece_index(piece), verified(false) {}

		uint32 piece_index;
		bool verified;
	};

	typedef multi_index_container<
		DownloadPieceInfo, 
		indexed_by<
			ordered_unique<
				member<DownloadPieceInfo, uint32, &DownloadPieceInfo::piece_index>
			>,
			ordered_non_unique<
				member<DownloadPieceInfo, uint32, &DownloadPieceInfo::priority> 
			>
		> 
	>DownloadingPieceSet;

	typedef DownloadingPieceSet::nth_index<0>::type PieceIndexView;
	typedef DownloadingPieceSet::nth_index<1>::type PiecePriorityView;

	struct PriorityComparer
	{
		bool operator()(const DownloadPieceInfo& lhs, const DownloadPieceInfo& rhs) const
		{
			return lhs.priority > rhs.priority;
		}
	};

	struct PieceIndexComparer
	{
		bool operator()(const FinishedPieceInfo& lhs, const FinishedPieceInfo& rhs) const
		{
			return lhs.piece_index < lhs.piece_index;
		}
	};

	typedef std::multiset<DownloadPieceInfo, PriorityComparer> PriorityPieceSet;
	typedef std::map<uint32, FinishedPieceInfo> FinishedPieceMap;

private:
	void ConstructPieceInfo(const TorrentSP& torrent);
	void PickBlocksFromDownloadingPieces(const boost::dynamic_bitset<>& filter, 
			uint32 num_blocks, PieceBlockVec& interesting_blocks);
	void PickBlocksFromWaitingPieces(const boost::dynamic_bitset<>& filter, 
			uint32 num_blocks, PieceBlockVec& interesting_blocks);
	void OnVerifyPiece(bool ok, const DiskIoJobSP& job);
	void FailToVerifyPieceProc(uint32 piece);
	void UpdatePiecePriority(uint32 piece, uint32 priority);
	void ReInitPieceInfo(DownloadPieceInfo& piece_info);
	DownloadPieceInfo ConstructDownloadPiece(const TorrentSP& torrent, uint32 piece);
	bool ShouldPieceStartDownloading(uint32 priority);

private:
	TorrentWP torrent_;

	boost::mutex piece_mutex_;
	PriorityPieceSet	waiting_pieces_;
	DownloadingPieceSet downloading_pieces_;
	FinishedPieceMap	finished_pieces_;
	
	uint32 verified_piece_num_;
	
	boost::scoped_ptr<PiecePriorityStrategy> strategy_;

	boost::dynamic_bitset<> piece_map_;
};

}

#endif

