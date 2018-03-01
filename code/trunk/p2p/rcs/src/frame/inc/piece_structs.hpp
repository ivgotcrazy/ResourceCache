/*------------------------------------------------------------------------------
 * �ļ���   : piece_structs.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.09.07
 * �ļ����� : ����piece��block�����ݽṹ����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/
#ifndef HEADER_PIECE_STRUCTS
#define HEADER_PIECE_STRUCTS

#include "bc_typedef.hpp"

namespace BroadCache
{

static const int kPiecePriorityMin = 0; //���ڴ����ȼ���piece�������˵������ᱻ���أ�
static const int kPiecePriorityNormal = 1; //���������ȼ�
static const int kPiecePriorityMax = 7; //�������ȼ�

/*------------------------------------------------------------------------------
 *��  ��: piece�е�block��������
 *��  ��: rosan
 *ʱ  ��: 2013.09.07
 -----------------------------------------------------------------------------*/
struct PieceBlock
{
	PieceBlock() : piece_index(0), block_index(0) {}
	PieceBlock(int p_index, int b_index) : piece_index(p_index), block_index(b_index) {}

	int piece_index; //piece���ļ��е�����
	int block_index; //block��piece�е�����

    //PieceBlock����С�ڲ�����
	bool operator<(PieceBlock const& b) const
	{
		if (piece_index < b.piece_index) return true;
		if (piece_index == b.piece_index) return block_index < b.block_index;
		return false;
	}

    //PieceBlock���ص��ڲ�����
	bool operator==(PieceBlock const& b) const
	{ return piece_index == b.piece_index && block_index == b.block_index; }

    //PieceBlock���ز����ڲ�����
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
 *��  ��: piece�е�block������ʱ�Ķ�̬��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.09.07
 -----------------------------------------------------------------------------*/
struct BlockInfo
{
	BlockInfo(): peer(0), num_peers(0), state(BLOCK_STATE_NONE) {}
    
	// ��block�Ǵ��ĸ�peer���ص�
    void* peer;
    
	// ���ڴ���ô��peer���ش�block������ô��PeerConnection������/������а�����block
    unsigned int num_peers:14;

    // block��״̬
    enum { BLOCK_STATE_NONE, BLOCK_STATE_REQUESTED, BLOCK_STATE_WRITING, BLOCK_STATE_FINISHED };
    unsigned int state:2;
};

/*------------------------------------------------------------------------------
 *��  ��: piece�����ض�̬��Ϣ
 *��  ��: rosan
 *ʱ  ��: 2013.09.07
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

    int index; //��piece���ļ��ڵ�����

    //��piece��������block��ϢΪ[block_begin, block_end)
    BlockInfo* block_begin; //��piece��������block��Ϣ��ʼָ��
	BlockInfo* block_end; //��piece��������block��Ϣ�Ľ���ָ��

    uint16 none;  // ��piece�д���none״̬��block��Ŀ
    uint16 requested;  // ��piece�д���requested״̬��block��Ŀ
    uint16 writing;  // ��piece�д���writing״̬��block��Ŀ
    uint16 finished;  // ��piece�д���finished״̬��block��Ŀ
};

}

#endif
