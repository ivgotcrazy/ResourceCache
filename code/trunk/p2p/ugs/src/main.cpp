/*#############################################################################
 * 文件名   : main.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : 入口函数
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <glog/logging.h>
#include "session_manager.hpp"
#include "ugs_config_parser.hpp"
#include "communicator.hpp"
#include "raw_data.hpp"

using namespace BroadCache;

int main()
{
    boost::asio::io_service ios;

	UgsConfigParser::GetInstance().Init();

    if (!RawDataSender::instance().Init("em1"))
    {
       LOG(INFO) << "Fail to create raw socket : " << strerror(errno);
    }

    Communicator::instance().Init(ios);

	SessionManager sm(ios);
	if (!sm.Start())
	{
		LOG(ERROR) << "Fail to start session manager";
		return -1;
	}

    ios.run();

	while(true)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}

	return 0;
}
