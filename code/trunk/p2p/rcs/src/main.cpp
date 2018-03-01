/*#############################################################################
 * �ļ���   : main.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : ϵͳ�����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <unistd.h>
#include <boost/lexical_cast.hpp>

#include "depend.hpp"
#include "session_manager.hpp"
#include "rcs_config_parser.hpp"
#include "communicator.hpp"
#include "tracker_request_manager.hpp"
#include "bcrm_server.hpp"
#include "monitor_manager.hpp"
#include "ip_filter.hpp"
#include "rcs_config.hpp"
#include "rcs_util.hpp"

using namespace BroadCache;

/*-----------------------------------------------------------------------------
 * ��  ��: ����glog��log����·��
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��26��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool SetGlogSavePath()
{
	std::string log_path_str;
	GET_RCS_CONFIG_STR("global.log.log-path", log_path_str);

	// ���log·�������ڣ����ȴ���·��
	fs::path log_path(log_path_str);
	if (!fs::exists(log_path))
	{
		if (!fs::create_directory(log_path))
		{
			LOG(ERROR) << "Fail to create log path" << log_path;
			return false;
		}
	}

	FLAGS_log_dir = log_path_str;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����glog��log����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��26��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool SetGlogLogLevel()
{
	uint32 log_level;
	GET_RCS_CONFIG_INT("global.log.log-level", log_level);

	if (log_level > 3) return false;

	FLAGS_minloglevel = log_level;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��glog
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��26��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool InitGlog()
{
	if (!SetGlogSavePath()) return false;

	if (!SetGlogLogLevel()) return false;

#ifdef BC_DEBUG
	FLAGS_stderrthreshold = google::INFO;
	FLAGS_colorlogtostderr = 1;
#endif

	google::InitGoogleLogging("rcs");

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��26��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Init(io_service& ios)
{
	RcsConfigParser::GetInstance().Init();

	if (!InitGlog()) return false;

    IpFilter::GetInstance().Init(IP_FILTER_PATH);

	Communicator::GetInstance().Init(ios);

	TrackerRequestManager::GetInstance().Init(ios);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��26��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Run()
{
	io_service ios;

	if (!Init(ios))
	{
		LOG(ERROR) << "Fail to initialize";
		return false;
	}

	SessionManager sm(ios);

	if (!sm.Run())
	{
		LOG(ERROR) << "Fail to run session manager";
		return false;
	}

	MonitorManager monitor(sm);
	//monitor.Run();

	BcrmServer bcrm_server(sm, monitor);
	if(!bcrm_server.Start(ios))
	{
		LOG(ERROR) << "Fail to start bcrm server";
		return false;
	}

	while (true)
	{
		ios.run();
		ios.reset();
	}

	return true;
}

int main(int argc, char* argv[])
{
/*
    if (-1 == daemon(0, 0))
    {
        LOG(INFO) << "deamon error";
        return -1;
    }
*/
	if (!Run())
	{
		LOG(ERROR) << "Fail to run program";
		return -1;
	}

    return 0;
}
