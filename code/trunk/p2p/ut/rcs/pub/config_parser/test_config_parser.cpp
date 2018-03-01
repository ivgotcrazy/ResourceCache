#include <gtest/gtest.h>  
#include <iostream> 

#include "config_parser.hpp"

using namespace std;

namespace BroadCache{

class ConfigParserTest: public testing::Test  
{  
	public:  
	 // 测试套统一初始化  init
	 virtual void SetUp(){
	   puts("SetUp()");
	   ConfigParser::GetInstance()->Init();
	 }  
	 virtual void TearDown(){
	   puts("TearDown()");
	 }  
};  



bool ParserTest(const string& module, const string& key,string& s){	
	return ConfigParser::GetInstance()->GetValue<string>(module,key,s);
}



TEST_F(ConfigParserTest,GetValue)
{
	string s;
	EXPECT_EQ(true,ParserTest("global","module.ppl",s));
	EXPECT_EQ(true,ParserTest("global","module.zzz",s));
	//EXPECT_EQ(ture,ParserTest("global","module.xl",s));
	//EXPECT_EQ(ture,ParserTest("global","module.xl",s));
}

}


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


