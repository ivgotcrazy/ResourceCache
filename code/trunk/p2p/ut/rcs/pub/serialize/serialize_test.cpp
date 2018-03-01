#include <gtest/gtest.h>
#include "serialize.hpp"

TEST(serialize, int)
{
    char buf[1024];
    
    int i = 888;
    BroadCache::Serializer serializer(buf);
    serializer & i;

    int j = 0;
    BroadCache::Unserializer unserializer(buf);
    unserializer & j;

    EXPECT_EQ(i, j);
}

TEST(serialize, string)
{
    char buf[1024];
    
    std::string s1("hello world");
    BroadCache::Serializer serializer(buf);
    serializer & s1;

    std::string s2;
    BroadCache::Unserializer unserializer(buf);
    unserializer & s2;

    EXPECT_EQ(s1, s2); 
}

TEST(serialize, cstring)
{
    char buf[1024];
    char a1[16] = "hello world";
    char a2[16];
    char* p1 = a1;
    char* p2 = a2;

    BroadCache::Serializer serializer(buf);
    serializer & p1;
    
    BroadCache::Unserializer unserializer(buf);
    unserializer & p2;
    
    EXPECT_EQ(std::strcmp(a1, a2), 0);
}

TEST(serializer, list)
{
    char buf[1024];

    std::list<int> l1;
    std::list<int> l2;
    l1.push_back(1);
    l1.push_back(2);

    BroadCache::Serializer serializer(buf);
    serializer & l1;

    BroadCache::Unserializer unserializer(buf);
    unserializer & l2;

    EXPECT_EQ(l1, l2);
}
