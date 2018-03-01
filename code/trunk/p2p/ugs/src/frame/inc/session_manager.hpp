/*#############################################################################
 * 文件名   : session_manager.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : SessionManager类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_SESSION_MANAGER
#define HEADER_SESSION_MANAGER

#include <vector>

#include <boost/noncopyable.hpp>
#include "ugs_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述：Session管理者
 * 作者：teck_zhou
 * 时间：2013年11月13日
 *****************************************************************************/
class SessionManager : public boost::noncopyable
{
public:
	explicit SessionManager(boost::asio::io_service& ios);

	bool Start();

private:
	bool Init();

private:
    boost::asio::io_service& ios_;
	std::vector<SessionSP> sessions_;
};

}

#endif
