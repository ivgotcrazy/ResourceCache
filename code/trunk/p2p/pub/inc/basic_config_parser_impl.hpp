/*#############################################################################
 * 文件名   : basic_config_parser_impl.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013-11-14
 * 文件描述 : basic_config_parser 头文件中模板函数的定义
 * ##########################################################################*/

#ifndef HEADER_CONFIG_PARSER_IMPL
#define HEADER_CONFIG_PARSER_IMPL

#include "depend.hpp"
#include "basic_config_parser.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 供GetValue调用，封装调用细节
 * 参  数: [in] ConfigType type 协议类型
 *         [in] const string& key  参数名
 *         [out] T &t 泛型返回的参数值
 * 返回值:
 * 修  改:
 * 时间 2013年08月19日
 * 作者 tom_liu
 * 描述 创建
 ----------------------------------------------------------------------------*/
template<typename T>
bool BasicConfigParser::GetValue(const std::string& skey, T &t)
{
	std::map<std::string, po::variable_value>::iterator iter;
	
	iter = vm_map_.find(skey);
	if (iter != vm_map_.end())
	{
		try
		{
			t = boost::any_cast<T>((iter->second).value());
		}
		catch (std::exception& e)
		{
			LOG(ERROR) << "Fail to get value | " << e.what();
			return false;
		}
	}

	return true;
}


}

#endif

