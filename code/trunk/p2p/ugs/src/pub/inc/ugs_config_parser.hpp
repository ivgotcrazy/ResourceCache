/*#############################################################################
 * 文件名   : rcs_config_parser.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013-11-13
 * 文件描述 : UgsConfigParser 头文件声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEAD_UGS_CONFIG_PARSER
#define HEAD_UGS_CONFIG_PARSER

#include "depend.hpp"
#include "ugs_typedef.hpp"
#include "basic_config_parser.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述：主要用于解析和读取配置文件的相关信息，唯一对外接口为  GetValue
 * 	    如要读取 bt 的配置文件的一个字段， Get("global.session.module",&t); 
 * 作者：tom_liu
 * 时间：2013/11/14
 *****************************************************************************/
class UgsConfigParser
{
public:
	void Init();

	template<typename T> 
	bool GetValue(const std::string& key, T &t);

	static UgsConfigParser& GetInstance();

private:
	enum UgsConfigType   // 协议顺序不能变
	{
		GLOBAL_CONFIG,
		BT_CONFIG,
		PPS_CONFIG,
		XL_CONFIG,
		PPL_CONFIG
	};

private:
	UgsConfigParser(){}

	void InitGlobalConfig();
	void InitProtocolConfig();

	bool ParseConfigTypeAndConfigItem(const std::string& key, UgsConfigType& module,
							std::string& skey);

	BasicConfigParserSP CreateBasicParser(UgsConfigType type, 
													po::options_description& option);
private:
	typedef std::map<UgsConfigType, BasicConfigParserSP> BasicParserMap;
	BasicParserMap basic_config_map_;	
};

}

#include "ugs_config_parser_impl.hpp"

#endif
