#include <gtest/gtest.h>
#include "msg.hpp"
#include "msg_dispatcher.hpp"

namespace BroadCache
{

class DispatcherTestMsg : public Msg
{
public:
    MSG_DECL(DispatcherTestMsg) 
};

MSG_IMPL(DispatcherTestMsg)

}

static int i = 0;

static void foo(const BroadCache::DispatcherTestMsg&)
{
    i = 1024;
}

TEST(MsgDispatcher, test)
{
    BroadCache::MsgDispatcher dispatcher;
    ASSERT_TRUE(dispatcher.RegisterCallback<BroadCache::DispatcherTestMsg>(&foo));
    
    BroadCache::DispatcherTestMsg dispatcher_test_msg;
    const BroadCache::Msg& msg = dispatcher_test_msg;
    ASSERT_TRUE(dispatcher.Dispatch(msg)); 
    
    EXPECT_EQ(i, 1024);

    EXPECT_TRUE(dispatcher.UnregisterCallback<BroadCache::DispatcherTestMsg>()); 
}

