/*#############################################################################
 * �ļ���   : ugs_config_parser.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��11��13��
 * ����ʱ�� : 2013��8��8��
 * �ļ����� : ��ȡ�����ļ���Ϣ������Ӧ�������ṩGetValue�Ƚӿڹ������������� 
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "ugs_config_parser.hpp"
#include "ugs_typedef.hpp"
#include "ugs_config.hpp"

namespace BroadCache
{

static int32 kProtocolnum = 3;  //֧�ֵ�Э����� , �������1������global 

static const char* kConfigPaths[] = { GLOBAL_CONFIG_PATH, BT_CONFIG_PATH, PPS_CONFIG_PATH };
static const char* kModuleNames[] = { "NULL", "global.protocol.bt", "global.protocol.pps" };
static const char* kProtocol[] = { "global", "bt", "pps" };

#define FILL_GLOBAL_CONFIG_OPTION(config_options) \
	config_options.add_options() \
	("common.domain",						po::value<std::string>(), "") \
	("common.capture-interface",			po::value<std::string>(), "") \
	("common.listen-address",				po::value<std::string>(), "") \
	("common.access-times-boundary",		po::value<uint32>(), "") \
	("common.access-rank-boundary",			po::value<uint32>(), "") \
	("common.hot-resource-strategy",	    po::value<std::string>(), "") \
	("common.peer-alive-time",				po::value<uint32>() 	, "") \
	("common.provide-local-peer",			po::value<std::string>(), "") \
	("common.max-local-peers-num",			po::value<uint32>() 	, "") \
	("protocol.bt",							po::value<std::string>(), "") \
	("protocol.pps",						po::value<std::string>(), "") \
	("log.log-path",				        po::value<std::string>(), "") \
	("log.log-level",				        po::value<std::string>(), "") \
	

#define FILL_BT_CONFIG_OPTION(config_options) \
	config_options.add_options() \
	("common.support-proxy-request", 		po::value<std::string>(), "") \
	("common.all-redirect", 				po::value<std::string>(), "") \
	("policy.provide-inner-peer", 		 	po::value<std::string>(), "") \
	("policy.resource-dispatch-policy",    	po::value<std::string>(), "") \
	("policy.hangup-outer-peer",    	 	po::value<std::string>(), "") \

#define FILL_PPS_CONFIG_OPTION(config_options) \
	config_options.add_options() \
	("common.support-proxy-request", 		po::value<std::string>(), "") \
	("common.all-redirect", 				po::value<std::string>(), "") \
	("policy.provide-inner-peer", 		 	po::value<std::string>(), "") \
	("policy.resource-dispatch-policy",    	po::value<std::string>(), "") \
	("policy.hangup-outer-peer",    	 	po::value<std::string>(), "") \

	
/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ����������ȡ�����ļ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UgsConfigParser::Init()
{
	InitGlobalConfig();
	InitProtocolConfig();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��global�����ļ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UgsConfigParser::InitGlobalConfig()
{
	po::options_description global_option("Configuration");
	BasicConfigParserSP global_parser = CreateBasicParser(GLOBAL_CONFIG, global_option);
	global_parser->ParseConfig();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ�����������ļ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void UgsConfigParser::InitProtocolConfig()
{
	BasicParserMap::iterator it = basic_config_map_.find(GLOBAL_CONFIG);
	if (it == basic_config_map_.end())
	{
		LOG(ERROR) << "Don't have the basic config parser";
		return;
	}
	BasicConfigParserSP global_parser = it->second; 

	bool callret = false;
	std::string support;
	for(int32 i=1; i<kProtocolnum; ++i) // i �� 1��ʼ�� 0 Ϊglobal�����߼���һ�� 
	{
		//�����ģ������on�� �����ζ�ȡ�������ļ�
		callret = GetValue(kModuleNames[i], support);
		if(callret && support == "yes")
		{	
			po::options_description option("Configuration");
			BasicConfigParserSP parser = CreateBasicParser(static_cast<UgsConfigType>(i),option);
			parser->ParseConfig();
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����BasicConfigParser����ʵ�ʽ���
 * ��  ��: [in]  type  Э������
 *		 [out] option Э����صĲ��������� 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��14��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
BasicConfigParserSP UgsConfigParser::CreateBasicParser(UgsConfigType type, po::options_description& option)
{
	BasicConfigParserSP basic_parser_;
	switch(type)
	{
		case GLOBAL_CONFIG:
			FILL_GLOBAL_CONFIG_OPTION(option);
			basic_parser_.reset(new BasicConfigParser(option, kConfigPaths[type]));
			break;
			
		case BT_CONFIG:
			FILL_BT_CONFIG_OPTION(option);
			basic_parser_.reset(new BasicConfigParser(option, kConfigPaths[type]));
			break;

		case PPS_CONFIG:
			FILL_BT_CONFIG_OPTION(option);
			basic_parser_.reset(new BasicConfigParser(option, kConfigPaths[type]));
			break;

		default:
			LOG(ERROR) << "Can't Support this type | type: " << type;
			break;
	}
	
	basic_config_map_.insert(std::make_pair(type, basic_parser_));
	return basic_parser_;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ַ���ת�������� "global.global.UGS"ת��  GLOBAL_CONFIG ��  "global.UGS"
 * ��  ��: [in]  key ת��֮ǰ���ַ���
 *         [out] module ����key����������
 *         [out] skey ת��֮��Ĳ��Ҳ���
 * ����ֵ: ת�������ַ����Ƿ�ɹ�
 * ��  ��: 
 * ʱ�� 2013��09��16��
 * ���� tom_liu
 * ���� ����
 ----------------------------------------------------------------------------*/
 bool UgsConfigParser::ParseConfigTypeAndConfigItem(const std::string&  key, 
 													UgsConfigType& module, std::string&  skey)
{
	bool ret = false;
	std::string::size_type pos = key.find('.');
	if (pos == key.npos)
	{
		LOG(ERROR) << "Can't find '.' in the key string";
		return ret;
	}
	std::string type = key.substr(0, pos);

	// i��0��ʼ,globalҲ�����е�һ���ж� 
	int32  i;
	for (i=0; i<kProtocolnum; ++i)
	{
		if (type == kProtocol[i])
		{
			module = static_cast<UgsConfigType>(i);
			break;
		}
	}
	
    // ������ҵ� kProtocolnum, �ͱ�ʾ��û�ҵ� 
	if (i == kProtocolnum)
	{
		LOG(ERROR) << "Don't have this module " << key;
		return ret;
	}
	
    ret = true;
	skey = key.substr(pos+1);
    //LOG(INFO)<< "SwitchKey | Module: " << module << ", skey: " << skey;
	return ret;
}

/*-----------------------------------------------------------------------------
 * ��  ��: UgsConfigParser �����ӿ�
 * ��  ��: 
 * ����ֵ: UgsConfigParser����
 * ��  ��:
 *   ʱ�� 2013��08��19��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
UgsConfigParser& UgsConfigParser::GetInstance()
{
	static UgsConfigParser instance;
    return instance;
}

}

