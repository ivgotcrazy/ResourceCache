/*#############################################################################
 * 文件名   : rcs_util.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : RCS公共使用函数
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_RCS_UTIL
#define HEADER_RCS_UTIL

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "peer_request.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

// 将ReadDataJob转换为PeerRequest
PeerRequest ToPeerRequest(const ReadDataJob& read_job);
PeerRequest ToPeerRequest(const WriteDataJob& write_job);

// 获取配置项
#define GET_RCS_CONFIG_INT(item, para) GET_CONFIG_INT(RcsConfigParser, item, para)
#define GET_RCS_CONFIG_STR(item, para) GET_CONFIG_STR(RcsConfigParser, item, para)
#define GET_RCS_CONFIG_BOOL(item, para) GET_CONFIG_BOOL(RcsConfigParser, item, para)

// shared pointer强转
#define SP_CAST(Type, shared_ptr) boost::dynamic_pointer_cast<Type>(shared_ptr)

}

#endif
