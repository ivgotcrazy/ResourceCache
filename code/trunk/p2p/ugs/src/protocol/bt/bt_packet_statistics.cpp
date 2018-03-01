/*##############################################################################
 * 文件名   : bt_packet_statistics.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : BtPacketStatistics类的实现类 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "bt_packet_statistics.hpp"

#include <boost/bind.hpp>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: BtPacketStatistics类的构造函数
 * 参  数: [in][out] ios IO服务对象
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
BtPacketStatistics::BtPacketStatistics(boost::asio::io_service& ios)
    : timer_(ios, boost::bind(&BtPacketStatistics::LogStatistics, this), 60)
{
    timer_.Start();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取某个bt数据包的统计信息
 * 参  数: [in] packet_type bt数据包类型
 * 返回值: 此bt数据包类型对应的统计信息
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
Counter& BtPacketStatistics::GetCounter(BtPacketType packet_type)
{
    return counters_[packet_type];
}

/*------------------------------------------------------------------------------
 * 描  述: 将统计信息写入日志文件中
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void BtPacketStatistics::LogStatistics()
{
    // bt数据包类型对应的字符串信息
    const char* const kPacketTypeDescriptions[BT_PACKET_TYPE_END] = 
        {
            "common",
            "HTTP get peer",
            "UDP get peer",
            "DHT get peer",
            "bt handshake",
            "HTTP get peer redirect",
            "UDP get peer redirect",
            "DHT get peer redirect",
            "bt handshake redirect"
        };

    int packet_type = BT_PACKET_TYPE_BEGIN;
    for (; packet_type != BT_PACKET_TYPE_END; ++packet_type)
    {
        LOG(INFO) << kPacketTypeDescriptions[packet_type] << " packet count : "
            << counters_[packet_type].Get();
        counters_[packet_type].Clear();  // 清空统计信息的计数器
    }
}

}  // namespace BroadCache
