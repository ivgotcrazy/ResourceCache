/*#############################################################################
 * 文件名   : rcs_config_parser.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013-8-19
 * 文件描述 : rcs_config_parser 头文件声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEADER_RCS_CONFIG_PARSER
#define HEADER_RCS_CONFIG_PARSER

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "basic_config_parser.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述：主要用于解析和读取配置文件的相关信息，唯一对外接口为  GetValue
 * 	    如要读取 bt 的配置文件的一个字段， Get("global.session.module",&t); 
 * 作者：tom_liu
 * 时间：2013/08/26
 *****************************************************************************/
class RcsConfigParser
{
public:
	void Init();

	template<typename T> 
	bool GetValue(const std::string& key, T &t);

	static RcsConfigParser& GetInstance();

private:
	enum RcsConfigType
	{
		GLOBAL_CONFIG,
		BT_CONFIG,
		PPS_CONFIG,	
		XL_CONFIG,
		PPL_CONFIG
	};
	
private:
	RcsConfigParser(){}

	void InitGlobalConfig();
	void InitProtocolConfig();

	bool ParseConfigTypeAndConfigItem(const std::string& key, RcsConfigType& module,
							std::string& skey);

	BasicConfigParserSP CreateBasicParser(RcsConfigType type, 
													po::options_description& option);
private:
	typedef std::map<RcsConfigType, BasicConfigParserSP> BasicParserMap;
	BasicParserMap basic_config_map_;
	
};

}

#include "rcs_config_parser_impl.hpp"

#endif 
