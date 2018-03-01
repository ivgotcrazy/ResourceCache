/*##############################################################################
 * 文件名   : pps_pkt_statistics.hpp
 * 创建人   : tom_liu 
 * 创建时间 : 2013.12.25
 * 文件描述 : PpsPacketStaticstics类的声明文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_PPS_PKT_STATISTICS
#define HEADER_PPS_PKT_STATISTICS

#include "timer.hpp"
#include "counter.hpp"
#include "pps_types.hpp"

namespace BroadCache
{

/*******************************************************************************
 * 描  述: 此类用于统计被处理/重定向的pps数据包情况
 * 作  者: tom_liu
 * 时  间: 2013.12.24
 ******************************************************************************/
class PpsPktStatistics
{
public:
    explicit PpsPktStatistics(boost::asio::io_service& ios);

    // 获取某个类型的pps数据包的统计信息
    Counter& GetCounter(PpsPacketType packet_type);

private:
    // 将bt数据包的统计信息写入日志中
    void LogStatistics();

private:
    Timer timer_;  // 写日志信息的定时器
    Counter counters_[PPS_PACKET_TYPE_END];  // bt数据包的统计信息
};

}  // namespace BroadCache

#endif  // HEADER_PPS_PKT_STATISTICS

