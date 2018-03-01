/*##############################################################################
 * 文件名   : bt_packet_statistics.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : BtPacketStaticstics类的声明文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_BT_PACKET_STATISTICS
#define HEADER_BT_PACKET_STATISTICS

#include "timer.hpp"
#include "counter.hpp"
#include "bt_types.hpp"

namespace BroadCache
{

/*******************************************************************************
 * 描  述: 此类用于统计被处理/重定向的bt数据包情况
 * 作  者: rosan
 * 时  间: 2013.11.23
 ******************************************************************************/
class BtPacketStatistics
{
public:
    explicit BtPacketStatistics(boost::asio::io_service& ios);

    // 获取某个类型的bt数据包的统计信息
    Counter& GetCounter(BtPacketType packet_type);

private:
    // 将bt数据包的统计信息写入日志中
    void LogStatistics();

private:
    Timer timer_;  // 写日志信息的定时器
    Counter counters_[BT_PACKET_TYPE_END];  // bt数据包的统计信息
};

}  // namespace BroadCache

#endif  // HEADER_BT_PACKET_STATISTICS
