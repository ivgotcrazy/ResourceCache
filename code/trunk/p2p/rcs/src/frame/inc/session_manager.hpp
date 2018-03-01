/*#############################################################################
 * �ļ���   : session_manager.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��8��
 * �ļ����� : SessionManager������
 * ##########################################################################*/

#ifndef HEAER_SESSION_MANAGER
#define HEAER_SESSION_MANAGER

#include "depend.hpp"
#include "session.hpp"
#include "disk_io_thread.hpp"



namespace BroadCache
{

/******************************************************************************
 * ����: Session������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class SessionManager : public boost::noncopyable
{
public:
	SessionManager(io_service& ios);
	~SessionManager();	
	bool Run();

	//��Դ������ش�����
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
