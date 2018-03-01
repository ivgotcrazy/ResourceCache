/*#############################################################################
 * 文件名   : rcs_config_parser.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月8日
 * 文件描述 : 读取配置文件信息并做相应解析，提供GetValue等接口供其他函数调用 
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "rcs_config_parser.hpp"
#include "rcs_config.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{

static int32 kProtocolnum = 3;  //支持的协议个数 , 个数会加1算上了global 

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
 * 描  述: 初始化函数，读取配置文件入口
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void RcsConfigParser::Init()
{
	InitGlobalConfig();
	InitProtocolConfig();
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化global配置文件
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void RcsConfigParser::InitGlobalConfig()
{
	po::options_description  global_option("Configuration");
	BasicConfigParserSP global_parser = CreateBasicParser(GLOBAL_CONFIG, global_option);
	global_parser->ParseConfig();
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化其他配置文件
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 tom_liu
 *   描述 创建
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
	for(int32 i = 1; i < kProtocolnum; ++i) // i 从 1开始， 0 为global处理逻辑不一样 
	{
		//如果子模块设置on， 则依次读取子配置文件
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
 * 描  述: 生产BasicConfigParser用于实际解析
 * 参  数: [in]  type  协议类型
 *         [out] option 协议相关的参数配置项 
 * 返回值:
 * 修  改:
 *   时间 2013年11月14日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 字符串转换函数， "global.global.UGS"转成  GLOBAL_CONFIG 和  "global.UGS"
 * 参  数: [in]  key 转换之前的字符串
 *         [out] module 查找key所属的类型
 *         [out] skey 转换之后的查找参数
 * 返回值: 转换查找字符串是否成功
 * 修  改: 
 * 时间 2013年09月16日
 * 作者 tom_liu
 * 描述 创建
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

	// i从0开始,global也是其中的一个判断 
	int32  i;
	for (i=0; i<kProtocolnum; ++i)
	{
		if (type == kProtocol[i])
		{
			module = static_cast<RcsConfigType>(i);
			break;
		}
	}
	
    // 如果查找到 kProtocolnum, 就表示还没找到 
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
 * 描  述: RcsConfigParser 单例接口
 * 参  数: 
 * 返回值: RcsConfigParser引用
 * 修  改:
 *   时间 2013年08月19日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
RcsConfigParser& RcsConfigParser::GetInstance()
{
	static RcsConfigParser instance;
    return instance;
}

}

