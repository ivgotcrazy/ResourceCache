#include <gtest/gtest.h>
#include "file.hpp"

using namespace BroadCache;

class FileTest : public testing::Test
{
protected:
    virtual void SetUp()
    {   
    }   

};

TEST_F(FileTest, DefaultConstructor)
{
    EXPECT_EQ(0, 1); 
}

