/*##############################################################################
 * 文件名   : pps_packet_statistics.cpp
 * 创建人   : tom_liu 
 * 创建时间 : 2013.12.24
 * 文件描述 : PpsPacketStatistics类的实现类 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "pps_pkt_statistics.hpp"

#include <boost/bind.hpp>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: PpsPktStatistics类的构造函数
 * 参  数: [in][out] ios IO服务对象
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
PpsPktStatistics::PpsPktStatistics(boost::asio::io_service& ios)
    : timer_(ios, boost::bind(&PpsPktStatistics::LogStatistics, this), 60)
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
Counter& PpsPktStatistics::GetCounter(PpsPacketType packet_type)
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
void PpsPktStatistics::LogStatistics()
{
    int packet_type = PPS_PACKET_TYPE_BEGIN;
    for (; packet_type != PPS_PACKET_TYPE_END; ++packet_type)
    {
       // LOG(INFO) << kPpsPktTypeDescriptions[packet_type] << " packet count : "
       //     << counters_[packet_type].Get();
        counters_[packet_type].Clear();  // 清空统计信息的计数器
    }
}

}  // namespace BroadCache

