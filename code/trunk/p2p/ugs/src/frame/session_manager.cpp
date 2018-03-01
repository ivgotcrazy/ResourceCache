/*#############################################################################
 * 文件名   : session_manager.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : SessionManager类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <glog/logging.h>

#include "session_manager.hpp"
#include "bt_session.hpp"
#include "pps_session.hpp"
#include "ugs_util.hpp"
#include "ugs_config_parser.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SessionManager::SessionManager(boost::asio::io_service& ios) : ios_(ios)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool SessionManager::Init()
{
	SessionSP session;

    bool protocol_switch = false;

    // 创建BT会话
    GET_UGS_CONFIG_BOOL("global.protocol.bt", protocol_switch);
    if (protocol_switch)
    {
	    session.reset(new BtSession(ios_));
	    if (!session->Init())
	    {
		    LOG(ERROR) << "Fail to init BT session";
		    return false;
	    }
	    sessions_.push_back(session);
    }

	GET_UGS_CONFIG_BOOL("global.protocol.pps", protocol_switch);
    if (protocol_switch)
    {
	    session.reset(new PpsSession(ios_));
	    if (!session->Init())
	    {
		    LOG(ERROR) << "Fail to init Pps session";
		    return false;
	    }
	    sessions_.push_back(session);
    }

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月13日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool SessionManager::Start()
{
	if (!Init())
	{
		LOG(ERROR) << "Fail to init session manager";
		return false;
	}

	for (SessionSP& session : sessions_)
	{
		if (!session->Start()) return false;
	}

	return true;
}


}
