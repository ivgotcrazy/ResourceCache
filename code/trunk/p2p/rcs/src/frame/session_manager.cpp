/*#############################################################################
 * 文件名   : session_manager.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月8日
 * 文件描述 : SessionManager类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "session_manager.hpp"

#include "rcs_util.hpp"
#include "bt_session.hpp"
#include "pps_session.hpp"
#include "communicator.hpp"
#include "rcs_config_parser.hpp"
#include "depend.hpp"
#include "info_hash.hpp"
#include "tracker_request_manager.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SessionManager::SessionManager(io_service& ios) 
	: ios_(ios)
{
	bool protocol_switch = false;
	GET_RCS_CONFIG_BOOL("global.protocol.bt", protocol_switch);

	if (protocol_switch)
	{
		SessionSP session(new BtSession());
		session_vec_.push_back(session);
	}


	protocol_switch = false;
	GET_RCS_CONFIG_BOOL("global.protocol.pps", protocol_switch);

	if (protocol_switch)
	{
		SessionSP session(new PpsSession());
		session_vec_.push_back(session);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SessionManager::~SessionManager()
{
	LOG(INFO) << ">>> Destructing session manager";
}

/*-----------------------------------------------------------------------------
 * 描  述: 停止接收新的连接
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月30日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SessionManager::StopAcceptNewConnection()
{
	LOG(INFO) << "Set all sessions to stop accepting new connection";

	for (SessionSP& session : session_vec_)
	{
		session->StopNewConnnetcion();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 恢复接收新的连接
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月30日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SessionManager::ResumeAcceptNewConnection()
{
	for (SessionSP& session : session_vec_)
	{
		session->ResumeNewConnnetcion();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 计算硬盘路径是否包含在session文件保存路径中
 * 参  数: [in] disk_name  硬盘路径
 		  [out] out_path_vec 包含硬盘路径的路径向量
 * 返回值:
 * 修  改:
 *   时间 2013年10月30日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void SessionManager::CalcShouldDeletePath(std::string disk_name,std::vector<std::string>& out_path_vec)
{
	//计算disk下有哪些存储路径
	for (SessionSP& session : session_vec_)
	{
		session->GetAdjustPath(disk_name,out_path_vec);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 判断路径是否在用
 * 参  数: [in] hash_str  路径名
 * 返回值: 是否被用 
 * 修  改:
 *   时间 2013年10月30日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool SessionManager::IsPathInUse(const std::string hash_str)
{
	// 判断这个路径是否已经存在
	bool ret = false;
	for (SessionSP& session : session_vec_)
	{
		ret = session->FindHashInTorrents(hash_str);
		if (ret) break;
	}	

	return ret;
}


/*-----------------------------------------------------------------------------
 * 描  述: 运行
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool SessionManager::Run()
{
	LOG(INFO) << "Run session manager...";

	for (SessionSP& session : session_vec_)
	{
		if (!session->Start())
		{
			LOG(ERROR) << "Fail to start session!!!";
			return false;
		}
	}
	
	return true;
}

}


