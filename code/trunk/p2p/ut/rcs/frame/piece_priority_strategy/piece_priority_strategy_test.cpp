#include "piece_priority_strategy.hpp"
#include <gtest/gtest.h> 
#include "piece_structs.hpp"

using namespace BroadCache;

TEST(PieceSequentialStrategy, min)
{
    PieceSequentialStrategy strategy(100, 9);
    EXPECT_TRUE(strategy.GetPriority(0) == PIECE_PRIORITY_MAX);    
}

TEST(PieceSequentialStrategy, max)
{
    PieceSequentialStrategy strategy(100, 9);
    EXPECT_TRUE(strategy.GetPriority(12) == PIECE_PRIORITY_NORMAL);    
}

TEST(PieceSequentialStrategy, mid)
{
    PieceSequentialStrategy strategy(100, 9);
    int priority = strategy.GetPriority(3);
    EXPECT_TRUE((priority < PIECE_PRIORITY_MAX) && (priority > PIECE_PRIORITY_NORMAL));  
}
