/*------------------------------------------------------------------------------
 * �ļ���   : piece_priority_strategy.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.09.06
 * �ļ����� : ����Ҫʵ��pieceѡ������ȼ����ԡ�
 *            �ļ�����PiecePriorityStrategy�༰������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#ifndef HEADER_PIECE_PRIORITY_STRATEGY
#define HEADER_PIECE_PRIORITY_STRATEGY 

#include "bc_typedef.hpp"

namespace BroadCache
{
/*------------------------------------------------------------------------------
 *��  ��: piece���ȼ����Ի���
 *��  ��: rosan
 *ʱ  ��: 2013.09.07
 -----------------------------------------------------------------------------*/
	class PiecePriorityStrategy
	{
	public:
		virtual ~PiecePriorityStrategy() {}

        //��ȡһ��piece�����ȼ�
		virtual int GetPriority(uint32 piece) = 0;
	};

/*------------------------------------------------------------------------------
 *��  ��: ʵ��piece��˳�����ز��ԣ������ļ�ǰ���piece�и��ߵ��������ȼ�
 *��  ��: rosan
 *ʱ  ��: 2013.09.07
 -----------------------------------------------------------------------------*/
	class PieceSequentialStrategy : public PiecePriorityStrategy
	{
	public:
		PieceSequentialStrategy(uint32 piece_num);

		virtual int GetPriority(uint32 piece) override;

	private:
		const uint32 pieces_; //�ļ���piece��Ŀ
	};
}

#endif
