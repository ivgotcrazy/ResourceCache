/*------------------------------------------------------------------------------
 * 文件名   : piece_structs.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.09.07
 * 文件描述 : 关于piece和block的数据结构定义
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#ifndef HEADER_PIECE_STRUCTS
#define HEADER_PIECE_STRUCTS

#include "bc_typedef.hpp"

namespace BroadCache
{

static const int kPiecePriorityMin = 0; //处于此优先级的piece将被过滤掉（不会被下载）
static const int kPiecePriorityNormal = 1; //正常的优先级
static const int kPiecePriorityMax = 7; //最大的优先级

/*------------------------------------------------------------------------------
 *描  述: piece中的block的索引类
 *作  者: rosan
 *时  间: 2013.09.07
 -----------------------------------------------------------------------------*/
struct PieceBlock
{
	PieceBlock() : piece_index(0), block_index(0) {}
	PieceBlock(int p_index, int b_index) : piece_index(p_index), block_index(b_index) {}

	int piece_index; //piece在文件中的索引
	int block_index; //block在piece中的索引

    //PieceBlock重载小于操作符
	bool operator<(PieceBlock const& b) const
	{
		if (piece_index < b.piece_index) return true;
		if (piece_index == b.piece_index) return block_index < b.block_index;
		return false;
	}

    //PieceBlock重载等于操作符
	bool operator==(PieceBlock const& b) const
	{ return piece_index == b.piece_index && block_index == b.block_index; }

    //PieceBlock重载不等于操作符
	bool operator!=(PieceBlock const& b) const
	{ return piece_index != b.piece_index || block_index != b.block_index; }
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const PieceBlock& pb)
{
	stream << "[" << pb.piece_index << ":" << pb.block_index << "]";
	return stream;
}

/*------------------------------------------------------------------------------
 *描  述: piece中的block在下载时的动态信息
 *作  者: rosan
 *时  间: 2013.09.07
 -----------------------------------------------------------------------------*/
struct BlockInfo
{
	BlockInfo(): peer(0), num_peers(0), state(BLOCK_STATE_NONE) {}
    
	// 此block是从哪个peer下载的
    void* peer;
    
	// 正在从这么多peer下载此block，即这么多PeerConnection的下载/请求队列包含此block
    unsigned int num_peers:14;

    // block的状态
    enum { BLOCK_STATE_NONE, BLOCK_STATE_REQUESTED, BLOCK_STATE_WRITING, BLOCK_STATE_FINISHED };
    unsigned int state:2;
};

/*------------------------------------------------------------------------------
 *描  述: piece的下载动态信息
 *作  者: rosan
 *时  间: 2013.09.07
 -----------------------------------------------------------------------------*/
struct DownloadingPiece
{
	DownloadingPiece(): index(0), 
                        block_begin(nullptr), 
                        block_end(nullptr), 
                        none(0),
                        requested(0),
                        writing(0), 
                        finished(0)
    {
    }

    int index; //此piece在文件在的索引

    //此piece所包含的block信息为[block_begin, block_end)
    BlockInfo* block_begin; //此piece所包含的block信息起始指针
	BlockInfo* block_end; //此piece所包含的block信息的结束指针

    uint16 none;  // 此piece中处于none状态的block数目
    uint16 requested;  // 此piece中处于requested状态的block数目
    uint16 writing;  // 此piece中处于writing状态的block数目
    uint16 finished;  // 此piece中处于finished状态的block数目
};

}

#endif
