/*#############################################################################
 * 文件名   : basic_config_parser.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年11月14日
 * 文件描述 : 读取配置文件信息并做相应解析，提供GetValue等接口供其他函数调用 
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#include "depend.hpp"
#include "basic_config_parser.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: [in] options  配置项
 * 		 [in] path 配置文件路径
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
BasicConfigParser::BasicConfigParser(po::options_description options, 
	const std::string path)
	: option_(options), config_path_(path)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析配置文件
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月13日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BasicConfigParser::ParseConfig()
{
	LOG(INFO)<< "Load configuration from " << config_path_;

	fs::path full_path(config_path_);
	if (!fs::exists(config_path_))
	{
		 LOG(INFO) << "Configure path(" << config_path_ << ")is't exsit!!!";
		 return ;
	}
	
	fs::ifstream ifs(config_path_);
	po::store(po::parse_config_file(ifs, option_), vm_map_);
	po::notify(vm_map_);
}

}


 

