#include "piece_picker.hpp"
#include <gtest/gtest.h>
#include "piece_priority_strategy.hpp"

using namespace BroadCache;

static const int blocks = 100;
static const int blocks_per_piece = 9;

static PieceSequentialStrategy strategy(blocks, blocks_per_piece);

/*TEST(PiecePicker, PieceFinished)
{ 
    PiecePicker picker(blocks, blocks_per_piece, &strategy); 
    
    picker.MarkPieceFinished(2);
    EXPECT_TRUE(picker.IsPieceFinished(2));
}*/

TEST(PiecePicker, BlockFinished)
{ 
    PiecePicker picker(blocks, blocks_per_piece, &strategy); 
    
    picker.MarkBlockFinished(PieceBlock(2, 5));
    EXPECT_TRUE(picker.IsBlockFinished(PieceBlock(2, 5)));
}

TEST(PiecePicker, PieceFinished_Blocks)
{ 
    PiecePicker picker(blocks, blocks_per_piece, &strategy); 
    
    int piece = 3;
    for(int i = 0; i < blocks_per_piece; ++i)
    {
        picker.MarkBlockFinished(PieceBlock(piece, i));
    }

    EXPECT_TRUE(picker.IsPieceFinished(piece));
}

TEST(PiecePicker, PickBlocks)
{
    boost::dynamic_bitset<> bs(12);
    bs.set();

    PiecePicker picker(blocks, blocks_per_piece, &strategy);
 
    std::vector<PieceBlock> v;
    picker.PickBlocks(bs, 10, v);
    
    ASSERT_TRUE(v.size() == 10);
    EXPECT_TRUE(v[0] == PieceBlock(0, 0));
    EXPECT_TRUE(v[9] == PieceBlock(1, 0)); 
}

TEST(PiecePicker, CanFinish)
{
}
