/*#############################################################################
 * 文件名   : rcs_config_parser_impl.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013-8-19
 * 文件描述 : rcs_config_parser 头文件中模板函数的定义
 * ##########################################################################*/

#ifndef HEADER_RCS_CONFIG_PARSER_IMPL
#define HEADER_RCS_CONFIG_PARSER_IMPL

#include "depend.hpp"
#include "rcs_config_parser.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * 描  述: 读取参数入口，用此函数进行参数读取
 * 参  数: [in] const string& key  参数名
 *         [out] T &t 泛型返回的参数值
 * 返回值: 获取参数是否成功
 * 修  改:
 * 时间 2013年08月19日
 * 作者 tom_liu
 * 描述 创建
 ----------------------------------------------------------------------------*/
template<typename T>
bool RcsConfigParser::GetValue(const std::string& key, T &t)
{
	bool ret = false;
	RcsConfigType type;
	std::string child_key;
	ParseConfigTypeAndConfigItem(key, type, child_key);

	BasicParserMap::iterator it = basic_config_map_.find(type);
	if (it == basic_config_map_.end())
	{
		LOG(ERROR) << "Can't find the basic config parser ";
	 	return ret;
	}
	
	ret = it->second->GetValue(child_key, t);	
	return ret;
}

}


#endif


