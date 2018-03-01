/*#############################################################################
 * �ļ���   : rcs_config_parser.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013-11-13
 * �ļ����� : UgsConfigParser ͷ�ļ�����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEAD_UGS_CONFIG_PARSER
#define HEAD_UGS_CONFIG_PARSER

#include "depend.hpp"
#include "ugs_typedef.hpp"
#include "basic_config_parser.hpp"

namespace BroadCache
{

/******************************************************************************
 * ��������Ҫ���ڽ����Ͷ�ȡ�����ļ��������Ϣ��Ψһ����ӿ�Ϊ  GetValue
 * 	    ��Ҫ��ȡ bt �������ļ���һ���ֶΣ� Get("global.session.module",&t); 
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/14
 *****************************************************************************/
class UgsConfigParser
{
public:
	void Init();

	template<typename T> 
	bool GetValue(const std::string& key, T &t);

	static UgsConfigParser& GetInstance();

private:
	enum UgsConfigType   // Э��˳���ܱ�
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
