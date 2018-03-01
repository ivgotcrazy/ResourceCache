/*##############################################################################
 * 文件名   : service_node_detector.cpp 
 * 创建人   : rosan 
 * 创建时间 : 2013.11.16
 * 文件描述 : 类ServiceNodeDetector的实现类 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "service_node_detector.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: ServiceNodeDetector类的构造函数
 * 参  数: [out] ios IO服务对象
 * 返回值:
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
ServiceNodeDetector::ServiceNodeDetector(boost::asio::io_service& ios) : 
    timer_(ios, boost::bind(&ServiceNodeDetector::OnTimer, this), kTimerInterval)
{
    timer_.Start();
}

/*------------------------------------------------------------------------------
 * 描  述: 收到keep-alive消息
 * 参  数: [in] endpoint RCS地址
 * 返回值:
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void ServiceNodeDetector::OnKeepAliveMsg(const EndPoint& endpoint)
{
    timeout_times_[endpoint] = 0;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取RCS超时的回调函数
 * 参  数: 
 * 返回值: RCS超时回调函数
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
ServiceNodeDetector::TimeoutHandler ServiceNodeDetector::GetTimeoutHandler() const
{
    return timeout_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置RCS超时的回调函数
 * 参  数: [in] handler RCS超时的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void ServiceNodeDetector::SetTimeoutHandler(const TimeoutHandler& handler)
{
    timeout_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 定时器回调函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void ServiceNodeDetector::OnTimer()
{
    auto i = timeout_times_.begin();
    while (i != timeout_times_.end())
    {
        ++(i->second);

        bool timeout = (i->second > kTimeoutMaxTimes);
        if (timeout && timeout_handler_)
        {
            timeout_handler_(i->first);
        }

        i = (timeout ? Erase(timeout_times_, i) : ++i);
    }
}

}  // namespace BroadCache
