/*#############################################################################
 * �ļ���   : session_manager.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : SessionManager������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_SESSION_MANAGER
#define HEADER_SESSION_MANAGER

#include <vector>

#include <boost/noncopyable.hpp>
#include "ugs_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * ������Session������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��11��13��
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
