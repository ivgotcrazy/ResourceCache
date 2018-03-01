/*#############################################################################
 * 文件名   : session_manager.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月8日
 * 文件描述 : SessionManager类声明
 * ##########################################################################*/

#ifndef HEAER_SESSION_MANAGER
#define HEAER_SESSION_MANAGER

#include "depend.hpp"
#include "session.hpp"
#include "disk_io_thread.hpp"



namespace BroadCache
{

/******************************************************************************
 * 描述: Session管理类
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
class SessionManager : public boost::noncopyable
{
public:
	SessionManager(io_service& ios);
	~SessionManager();	
	bool Run();

	//资源调度相关处理函数
	void StopAcceptNewConnection();
	void ResumeAcceptNewConnection();
	void CalcShouldDeletePath(std::string disk_name,std::vector<std::string>& out_path_vec);
	bool IsPathInUse(const std::string hash_str);

private:
	std::vector<SessionSP> session_vec_;
	io_service&	ios_;

	friend class GetBcrmInfo;	
};

}

#endif
