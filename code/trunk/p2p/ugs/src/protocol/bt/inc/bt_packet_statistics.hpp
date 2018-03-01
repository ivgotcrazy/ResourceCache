/*##############################################################################
 * �ļ���   : bt_packet_statistics.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : BtPacketStaticstics��������ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_BT_PACKET_STATISTICS
#define HEADER_BT_PACKET_STATISTICS

#include "timer.hpp"
#include "counter.hpp"
#include "bt_types.hpp"

namespace BroadCache
{

/*******************************************************************************
 * ��  ��: ��������ͳ�Ʊ�����/�ض����bt���ݰ����
 * ��  ��: rosan
 * ʱ  ��: 2013.11.23
 ******************************************************************************/
class BtPacketStatistics
{
public:
    explicit BtPacketStatistics(boost::asio::io_service& ios);

    // ��ȡĳ�����͵�bt���ݰ���ͳ����Ϣ
    Counter& GetCounter(BtPacketType packet_type);

private:
    // ��bt���ݰ���ͳ����Ϣд����־��
    void LogStatistics();

private:
    Timer timer_;  // д��־��Ϣ�Ķ�ʱ��
    Counter counters_[BT_PACKET_TYPE_END];  // bt���ݰ���ͳ����Ϣ
};

}  // namespace BroadCache

#endif  // HEADER_BT_PACKET_STATISTICS
