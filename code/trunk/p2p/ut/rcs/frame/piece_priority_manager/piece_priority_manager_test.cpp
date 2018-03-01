#include "piece_priority_manager.hpp"
#include <gtest/gtest.h>

using namespace BroadCache;

static const int piece = 5;
static const int priority = 6;

TEST(PiecePriorityManager, Add)
{
    PiecePriorityManager manager;

    EXPECT_TRUE(manager.Add(piece, priority));
    EXPECT_TRUE(!manager.Add(piece, priority));
}

TEST(PiecePriorityManager, Remove)
{
    PiecePriorityManager manager;

    ASSERT_TRUE(manager.Add(piece, priority));
    EXPECT_TRUE(manager.Remove(piece));
    EXPECT_TRUE(!manager.Remove(piece));
}



TEST(PiecePriorityManager, Update)
{
    PiecePriorityManager manager;

    ASSERT_TRUE(manager.Add(piece, priority));
    EXPECT_TRUE(manager.Update(piece, priority + 1));
    EXPECT_TRUE(manager.Get(piece) == priority + 1);
    EXPECT_TRUE(!manager.Update(piece + 1, priority));
}

TEST(PiecePriorityManager, FuzzyUpdate)
{
    PiecePriorityManager manager;

    manager.FuzzyUpdate(piece, priority);
    ASSERT_TRUE(manager.Has(piece));
    EXPECT_TRUE(manager.Get(piece) == priority); 

    manager.FuzzyUpdate(piece, priority + 1);
    ASSERT_TRUE(manager.Has(piece));
    EXPECT_TRUE(manager.Get(piece) == priority + 1); 
}

TEST(PiecePriorityManager, Has)
{
    PiecePriorityManager manager;
    ASSERT_TRUE(manager.Add(piece, priority));
    EXPECT_TRUE(manager.Has(piece));
}

TEST(PiecePriorityManager, Get)
{
    PiecePriorityManager manager;
    ASSERT_TRUE(manager.Add(piece, priority));
    EXPECT_TRUE(manager.Get(piece) == priority);
}

TEST(PiecePriorityManager, Clear)
{
    PiecePriorityManager manager;
    ASSERT_TRUE(manager.Add(piece, priority));
    manager.Clear();
    EXPECT_TRUE(!manager.Has(piece));
}

TEST(PiecePriorityManager, Sort)
{
    boost::dynamic_bitset<> bs(3);
    bs[0] = true;
    bs[1] = false;
    bs[2] = true;

    PiecePriorityManager manager;
    manager.Add(0, 101);
    manager.Add(1, 25);
    manager.Add(2, 177);

    std::vector<int> v = manager.Sort(bs);
    ASSERT_TRUE(v.size() == 2);
    EXPECT_TRUE(v[0] == 2);
    EXPECT_TRUE(v[1] == 0);
}
