/*#############################################################################
 * �ļ���   : rcs_config_parser.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013-8-19
 * �ļ����� : rcs_config_parser ͷ�ļ�����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��������Ҫ���ڽ����Ͷ�ȡ�����ļ��������Ϣ��Ψһ����ӿ�Ϊ  GetValue
 * 	    ��Ҫ��ȡ bt �������ļ���һ���ֶΣ� Get("global.session.module",&t); 
 * ���ߣ�tom_liu
 * ʱ�䣺2013/08/26
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
