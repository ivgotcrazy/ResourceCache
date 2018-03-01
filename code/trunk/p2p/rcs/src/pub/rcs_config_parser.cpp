/*#############################################################################
 * �ļ���   : rcs_config_parser.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��8��
 * �ļ����� : ��ȡ�����ļ���Ϣ������Ӧ�������ṩGetValue�Ƚӿڹ������������� 
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "rcs_config_parser.hpp"
#include "rcs_config.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{

static int32 kProtocolnum = 3;  //֧�ֵ�Э����� , �������1������global 

static const char* kConfigPaths[] = {GLOBAL_CONFIG_PATH, BT_CONFIG_PATH, PPS_CONFIG_PATH};
static const char* kProtocolNames[] = {"NULL", "global.protocol.bt", "global.protocol.pps"};
static const char* kProtocol[] = {"global", "bt", "pps"};

#define FILL_GLOBAL_CONFIG_OPTION(config_options) \
	config_options.add_options() \
	("common.ugs",								  		po::value<std::string>(), "") \
	("common.bcrm-listen-port",					  		po::value<std::string>(), "") \
	("protocol.bt",								  		po::value<std::string>(), "") \
	("protocol.pps",								  	po::value<std::string>(), "") \
	("session.socket-thread-number",			  		po::value<uint32>(),	  "") \
	("torrent.max-alive-time-without-connection", 		po::value<uint32>(),	  "") \
	("log.log-level",							  		po::value<uint32>(),	  "") \
	("log.log-path",							  		po::value<std::string>(), "") \
	("peerconnection.max-alive-time-without-traffic",	po::value<uint32>(),	  "");

#define FILL_BT_CONFIG_OPTION(config_options) \
	config_options.add_options() \
	("common.listen-addr",                            po::value<std::string>(), "") \
	("common.save-path",                              po::value<std::string>(), "") \
	("policy.support-cache-proxy",					  po::value<std::string>(), "") \
	("policy.enable-cached-rcs",					  po::value<std::string>(), "") \
	("distributed-download.enable-inner-proxy",		  po::value<std::string>(), "") \
	("distributed-download.inner-proxy-num",		  po::value<uint32>(),		"") \
	("distributed-download.enable-outer-proxy",		  po::value<std::string>(), "") \
	("distributed-download.outer-proxy-num",		  po::value<uint32>(),		"") \
	("distributed-download.download-speed-threshold", po::value<uint32>(),		"");

#define FILL_PPS_CONFIG_OPTION(config_options) \
	config_options.add_options() \
	("common.listen-addr",                            po::value<std::string>(), "") \
	("common.save-path",                              po::value<std::string>(), "") \
	("policy.support-cache-proxy",					  po::value<std::string>(), "") \
	("policy.enable-cached-rcs",					  po::value<std::string>(), "") \
	("distributed-download.enable-inner-proxy",		  po::value<std::string>(), "") \
	("distributed-download.inner-proxy-num",		  po::value<uint32>(),		"") \
	("distributed-download.enable-outer-proxy",		  po::value<std::string>(), "") \
	("distributed-download.outer-proxy-num",		  po::value<uint32>(),		"") \
	("distributed-download.download-speed-threshold", po::value<uint32>(),		"");

	

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ����������ȡ�����ļ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void RcsConfigParser::Init()
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
void RcsConfigParser::InitGlobalConfig()
{
	po::options_description  global_option("Configuration");
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
void RcsConfigParser::InitProtocolConfig()
{
	BasicParserMap::iterator it = basic_config_map_.find(GLOBAL_CONFIG);
	if (it == basic_config_map_.end())
	{
		LOG(ERROR) << "Fail to find global configuration";
		return;
	}
	BasicConfigParserSP global_parser = it->second; 

	bool result = false;
	std::string support;
	for(int32 i = 1; i < kProtocolnum; ++i) // i �� 1��ʼ�� 0 Ϊglobal�����߼���һ�� 
	{
		//�����ģ������on�� �����ζ�ȡ�������ļ�
		result = GetValue(kProtocolNames[i], support);
		if(result && support == "yes")
		{	
			po::options_description option("Configuration");
			BasicConfigParserSP parser = CreateBasicParser(static_cast<RcsConfigType>(i), option);
			parser->ParseConfig();
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����BasicConfigParser����ʵ�ʽ���
 * ��  ��: [in]  type  Э������
 *         [out] option Э����صĲ��������� 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��14��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
BasicConfigParserSP RcsConfigParser::CreateBasicParser(RcsConfigType type, 
														po::options_description& option)
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
			LOG(ERROR) << "Invalid configuration";
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
bool RcsConfigParser::ParseConfigTypeAndConfigItem(const std::string& key, RcsConfigType& module, 
										std::string& skey)
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
			module = static_cast<RcsConfigType>(i);
			break;
		}
	}
	
    // ������ҵ� kProtocolnum, �ͱ�ʾ��û�ҵ� 
	if (i == kProtocolnum)
	{
		LOG(ERROR) << "Don't have this module | key:" << key;
		return ret;
	}
	
    ret = true;
	skey = key.substr(pos + 1);
    //LOG(INFO)<< "SwitchKey | Module: " << module << ", skey: " << skey;
	return ret;
}

/*-----------------------------------------------------------------------------
 * ��  ��: RcsConfigParser �����ӿ�
 * ��  ��: 
 * ����ֵ: RcsConfigParser����
 * ��  ��:
 *   ʱ�� 2013��08��19��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
RcsConfigParser& RcsConfigParser::GetInstance()
{
	static RcsConfigParser instance;
    return instance;
}

}

