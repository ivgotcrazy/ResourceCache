#include <gtest/gtest.h>
#include "bencode.hpp"

using namespace BroadCache;

TEST(Bencode, size_type)
{
    BencodeEntry e1 = 2013;
    std::string s = Bencode(e1);

    BencodeEntry e2 = Bdecode(s);

    EXPECT_EQ(e1, e2);
}

TEST(Bencode, string)
{
    BencodeEntry e1 = std::string("hello world");
    std::string s = Bencode(e1);

    BencodeEntry e2 = Bdecode(s);

    EXPECT_EQ(e1, e2);
}

TEST(Bencode, list)
{
    BencodeEntry e1;
    e1.List().push_back(BencodeEntry(2013));
    e1.List().push_back(BencodeEntry(std::string("hello world")));

    std::string s = Bencode(e1);

    BencodeEntry e2 = Bdecode(s);

    EXPECT_EQ(e1, e2);
}

TEST(Bencode, map)
{
    BencodeEntry e1;
    e1.Dict().insert(std::make_pair("key1", BencodeEntry(2013)));
    e1.Dict().insert(std::make_pair("key2", BencodeEntry(std::string("hello world"))));

    std::string s = Bencode(e1);

    BencodeEntry e2 = Bdecode(s);

    EXPECT_EQ(e1, e2);
}

TEST(Bencode, list_map)
{
    BencodeEntry e1;
    e1.List().push_back(BencodeEntry(2013));
    e1.List().push_back(BencodeEntry(std::string("hello world")));

    BencodeEntry e3;
    e3.Dict().insert(std::make_pair("key1", BencodeEntry(2013)));
    e3.Dict().insert(std::make_pair("key2", BencodeEntry(std::string("hello world"))));

    std::string s = Bencode(e1);

    BencodeEntry e2 = Bdecode(s);

    EXPECT_EQ(e1, e2);
}

TEST(Bencode, map_list)
{
    BencodeEntry e1;
    e1.Dict().insert(std::make_pair("key1", BencodeEntry(2013)));
    e1.Dict().insert(std::make_pair("key2", BencodeEntry(std::string("hello world"))));

    BencodeEntry e3;
    e3.List().push_back(BencodeEntry(2014));
    e3.List().push_back(BencodeEntry(std::string("session")));
    
    e1.Dict().insert(std::make_pair("key3", e3));

    std::string s = Bencode(e1);

    BencodeEntry e2 = Bdecode(s);

    EXPECT_EQ(e1, e2);
}
