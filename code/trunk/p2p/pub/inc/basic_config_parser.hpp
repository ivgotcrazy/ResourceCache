/*#############################################################################
 * 文件名   : basic_config_parser.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013-11-14
 * 文件描述 : BasicConfigParser 头文件声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/


#ifndef HEADER_BASIC_CONFIG_PARSER
#define HEADER_BASIC_CONFIG_PARSER

namespace BroadCache
{

namespace po = boost::program_options;
namespace fs = boost::filesystem;

class BasicConfigParser
{
public:
	BasicConfigParser(po::options_description options, const std::string path);
	
	void ParseConfig();

	template<typename T> 
	bool GetValue(const std::string& key, T &t);
	
private:
	po::options_description option_;
	po::variables_map vm_map_;
	std::string config_path_;
};

}

#include "basic_config_parser_impl.hpp"

#endif

