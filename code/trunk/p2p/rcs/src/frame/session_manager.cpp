/*#############################################################################
 * �ļ���   : session_manager.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��8��
 * �ļ����� : SessionManager��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
SessionManager::~SessionManager()
{
	LOG(INFO) << ">>> Destructing session manager";
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣ�����µ�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��30��
 *   ���� tom_liu
 *   ���� ����
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
 * ��  ��: �ָ������µ�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��30��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SessionManager::ResumeAcceptNewConnection()
{
	for (SessionSP& session : session_vec_)
	{
		session->ResumeNewConnnetcion();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Ӳ��·���Ƿ������session�ļ�����·����
 * ��  ��: [in] disk_name  Ӳ��·��
 		  [out] out_path_vec ����Ӳ��·����·������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��30��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void SessionManager::CalcShouldDeletePath(std::string disk_name,std::vector<std::string>& out_path_vec)
{
	//����disk������Щ�洢·��
	for (SessionSP& session : session_vec_)
	{
		session->GetAdjustPath(disk_name,out_path_vec);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ж�·���Ƿ�����
 * ��  ��: [in] hash_str  ·����
 * ����ֵ: �Ƿ��� 
 * ��  ��:
 *   ʱ�� 2013��10��30��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool SessionManager::IsPathInUse(const std::string hash_str)
{
	// �ж����·���Ƿ��Ѿ�����
	bool ret = false;
	for (SessionSP& session : session_vec_)
	{
		ret = session->FindHashInTorrents(hash_str);
		if (ret) break;
	}	

	return ret;
}


/*-----------------------------------------------------------------------------
 * ��  ��: ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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


