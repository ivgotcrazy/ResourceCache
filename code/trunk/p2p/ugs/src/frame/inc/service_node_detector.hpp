/*##############################################################################
 * 文件名   : service_node_detector.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.15
 * 文件描述 : 类ServiceNodeDetector的声明文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_SERVICE_NODE_DETECTOR
#define HEADER_SERVICE_NODE_DETECTOR

#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "timer.hpp"

namespace BroadCache
{

/*******************************************************************************
 *描  述: 用于监测RCS存活状态
 *作  者: rosan
 *时  间: 2013.11.15
 ******************************************************************************/
class ServiceNodeDetector
{
public:
    typedef boost::function<void(const EndPoint&)> TimeoutHandler;  // 检测到RCS超时

public:
    ServiceNodeDetector(boost::asio::io_service& ios);

    // 收到keep-alive消息
    void OnKeepAliveMsg(const EndPoint& endpoint);

    // 获取RCS超时的回调函数
    TimeoutHandler GetTimeoutHandler() const;

    // 设置RCS超时的回调函数
    void SetTimeoutHandler(const TimeoutHandler& handler);

private:
    // 定时器回调函数
    void OnTimer();

private:
    static const uint32 kTimerInterval = 3;  // 定时器时间间隔（单位为秒）
    static const uint32 kTimeoutMaxTimes = 3;  // RCS超时最大次数（超过此次数就认为RCS断线）

    Timer timer_;  // 检测RCS存活状态的定时器
    TimeoutHandler timeout_handler_;  // 检测到RCS超时
    boost::unordered_map<EndPoint, uint32> timeout_times_;  // RCS超时次数
};

}  // namespace BroadCache

#endif  // HEADER_SERVICE_NODE_DETECTOR 
