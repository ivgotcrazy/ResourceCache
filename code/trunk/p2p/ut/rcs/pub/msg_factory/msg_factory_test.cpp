#include <gtest/gtest.h>
#include "msg_factory.hpp"
#include "msg.hpp"

namespace BroadCache 
{

class MyMsg : public Msg
{
public:
    MSG_DECL(MyMsg)
    MSG_SERIALIZE(id & name)

    int id;
    std::string name;
};

MSG_IMPL(MyMsg)

bool operator==(const MyMsg& lhs, const MyMsg& rhs)
{
    return lhs.id == rhs.id && lhs.name == rhs.name; 
}

}

TEST(msg_factory, test)
{
    static const size_t BUF_SIZE = 1024;
    char buf[BUF_SIZE];

    BroadCache::MyMsg my_msg;
    my_msg.id = 5;
    my_msg.name = "kitty";

    ASSERT_EQ(BroadCache::MsgFactory::Pack(&my_msg, buf, BUF_SIZE),
        BroadCache::MsgFactory::NO_ERROR);

    BroadCache::Msg* msg = BroadCache::MsgFactory::Unpack(buf, BUF_SIZE);
    ASSERT_TRUE(msg != NULL);
    
    BroadCache::MyMsg* pmy_msg = dynamic_cast<BroadCache::MyMsg*>(msg);
    ASSERT_TRUE(pmy_msg != NULL);

    EXPECT_EQ(my_msg, *pmy_msg);
}
