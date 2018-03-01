/*------------------------------------------------------------------------------
 * 文件名   : piece_priority_strategy.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.09.06
 * 文件描述 : 此主要实现piece选择的优先级策略。
 *            文件包含PiecePriorityStrategy类及其子类
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#ifndef HEADER_PIECE_PRIORITY_STRATEGY
#define HEADER_PIECE_PRIORITY_STRATEGY 

#include "bc_typedef.hpp"

namespace BroadCache
{
/*------------------------------------------------------------------------------
 *描  述: piece优先级策略基类
 *作  者: rosan
 *时  间: 2013.09.07
 -----------------------------------------------------------------------------*/
	class PiecePriorityStrategy
	{
	public:
		virtual ~PiecePriorityStrategy() {}

        //获取一个piece的优先级
		virtual int GetPriority(uint32 piece) = 0;
	};

/*------------------------------------------------------------------------------
 *描  述: 实现piece的顺序下载策略，即在文件前面的piece有更高的下载优先级
 *作  者: rosan
 *时  间: 2013.09.07
 -----------------------------------------------------------------------------*/
	class PieceSequentialStrategy : public PiecePriorityStrategy
	{
	public:
		PieceSequentialStrategy(uint32 piece_num);

		virtual int GetPriority(uint32 piece) override;

	private:
		const uint32 pieces_; //文件的piece数目
	};
}

#endif
