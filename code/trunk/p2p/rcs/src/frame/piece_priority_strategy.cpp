/*------------------------------------------------------------------------------
 * �ļ���   : piece_priority_strategy.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.09.06
 * �ļ����� : ʵ��PiecePriorityStrategy������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#include "piece_priority_strategy.hpp"
#include "rcs_typedef.hpp"
#include "piece_structs.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: ��PieceSequentialStrategy�Ĺ��캯��
 * ��  ��: [in] blocks �����ļ���block��Ŀ
 *         [in] blocks_per_piece һ��piece����block����Ŀ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.06
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
PieceSequentialStrategy::PieceSequentialStrategy(uint32 piece_num) : pieces_(piece_num)
{
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ڻ�ȡָ��piece�����ȼ�
 * ��  ��: [in] piece piece������ֵ
 * ����ֵ: piece�����ȼ�
 * ��  ��:
 *   ʱ�� 2013.09.06
 *   ���� rosan
 *   ���� ����
 *        ˳�����ز��ԣ����ļ���Խ��ǰ��piece,����Խ�ߵ����ȼ�
 *        ��piece������Ϊpiece_ - 1ʱ,���ȼ�Ϊ���PIECE_PRIORITY_NORMAL
 *        ��piece������Ϊ0ʱ�����ȼ�Ϊ���PIECE_PRIORITY_MAX - 1
 -----------------------------------------------------------------------------*/
int PieceSequentialStrategy::GetPriority(uint32 piece)
{
	return (pieces_ - piece - 1) * ((kPiecePriorityMax - 1) - kPiecePriorityNormal) / (pieces_ - 1)
		+ kPiecePriorityNormal;
}

}
