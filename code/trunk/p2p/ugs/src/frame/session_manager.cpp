/*#############################################################################
 * �ļ���   : session_manager.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : SessionManager��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SessionManager::SessionManager(boost::asio::io_service& ios) : ios_(ios)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool SessionManager::Init()
{
	SessionSP session;

    bool protocol_switch = false;

    // ����BT�Ự
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
 * ��  ��: ����
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
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
