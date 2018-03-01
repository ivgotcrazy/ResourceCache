/*------------------------------------------------------------------------------
 * 文件名   : piece_priority_strategy.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.09.06
 * 文件描述 : 实现PiecePriorityStrategy的子类
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#include "piece_priority_strategy.hpp"
#include "rcs_typedef.hpp"
#include "piece_structs.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 类PieceSequentialStrategy的构造函数
 * 参  数: [in] blocks 下载文件的block数目
 *         [in] blocks_per_piece 一个piece包含block的数目
 * 返回值:
 * 修  改:
 *   时间 2013.09.06
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
PieceSequentialStrategy::PieceSequentialStrategy(uint32 piece_num) : pieces_(piece_num)
{
}

/*------------------------------------------------------------------------------
 * 描  述: 用于获取指定piece的优先级
 * 参  数: [in] piece piece的索引值
 * 返回值: piece的优先级
 * 修  改:
 *   时间 2013.09.06
 *   作者 rosan
 *   描述 创建
 *        顺序下载策略，在文件中越靠前的piece,具有越高的优先级
 *        当piece的索引为piece_ - 1时,优先级为最低PIECE_PRIORITY_NORMAL
 *        当piece的索引为0时，优先级为最高PIECE_PRIORITY_MAX - 1
 -----------------------------------------------------------------------------*/
int PieceSequentialStrategy::GetPriority(uint32 piece)
{
	return (pieces_ - piece - 1) * ((kPiecePriorityMax - 1) - kPiecePriorityNormal) / (pieces_ - 1)
		+ kPiecePriorityNormal;
}

}
