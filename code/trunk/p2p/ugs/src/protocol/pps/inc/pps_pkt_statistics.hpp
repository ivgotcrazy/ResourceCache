/*##############################################################################
 * �ļ���   : pps_pkt_statistics.hpp
 * ������   : tom_liu 
 * ����ʱ�� : 2013.12.25
 * �ļ����� : PpsPacketStaticstics��������ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_PPS_PKT_STATISTICS
#define HEADER_PPS_PKT_STATISTICS

#include "timer.hpp"
#include "counter.hpp"
#include "pps_types.hpp"

namespace BroadCache
{

/*******************************************************************************
 * ��  ��: ��������ͳ�Ʊ�����/�ض����pps���ݰ����
 * ��  ��: tom_liu
 * ʱ  ��: 2013.12.24
 ******************************************************************************/
class PpsPktStatistics
{
public:
    explicit PpsPktStatistics(boost::asio::io_service& ios);

    // ��ȡĳ�����͵�pps���ݰ���ͳ����Ϣ
    Counter& GetCounter(PpsPacketType packet_type);

private:
    // ��bt���ݰ���ͳ����Ϣд����־��
    void LogStatistics();

private:
    Timer timer_;  // д��־��Ϣ�Ķ�ʱ��
    Counter counters_[PPS_PACKET_TYPE_END];  // bt���ݰ���ͳ����Ϣ
};

}  // namespace BroadCache

#endif  // HEADER_PPS_PKT_STATISTICS

