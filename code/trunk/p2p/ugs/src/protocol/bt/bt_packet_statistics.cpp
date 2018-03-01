/*##############################################################################
 * �ļ���   : bt_packet_statistics.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : BtPacketStatistics���ʵ���� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "bt_packet_statistics.hpp"

#include <boost/bind.hpp>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: BtPacketStatistics��Ĺ��캯��
 * ��  ��: [in][out] ios IO�������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
BtPacketStatistics::BtPacketStatistics(boost::asio::io_service& ios)
    : timer_(ios, boost::bind(&BtPacketStatistics::LogStatistics, this), 60)
{
    timer_.Start();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡĳ��bt���ݰ���ͳ����Ϣ
 * ��  ��: [in] packet_type bt���ݰ�����
 * ����ֵ: ��bt���ݰ����Ͷ�Ӧ��ͳ����Ϣ
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
Counter& BtPacketStatistics::GetCounter(BtPacketType packet_type)
{
    return counters_[packet_type];
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ͳ����Ϣд����־�ļ���
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtPacketStatistics::LogStatistics()
{
    // bt���ݰ����Ͷ�Ӧ���ַ�����Ϣ
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
        counters_[packet_type].Clear();  // ���ͳ����Ϣ�ļ�����
    }
}

}  // namespace BroadCache
