/*##############################################################################
 * �ļ���   : pps_packet_statistics.cpp
 * ������   : tom_liu 
 * ����ʱ�� : 2013.12.24
 * �ļ����� : PpsPacketStatistics���ʵ���� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "pps_pkt_statistics.hpp"

#include <boost/bind.hpp>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: PpsPktStatistics��Ĺ��캯��
 * ��  ��: [in][out] ios IO�������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
PpsPktStatistics::PpsPktStatistics(boost::asio::io_service& ios)
    : timer_(ios, boost::bind(&PpsPktStatistics::LogStatistics, this), 60)
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
Counter& PpsPktStatistics::GetCounter(PpsPacketType packet_type)
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
void PpsPktStatistics::LogStatistics()
{
    int packet_type = PPS_PACKET_TYPE_BEGIN;
    for (; packet_type != PPS_PACKET_TYPE_END; ++packet_type)
    {
       // LOG(INFO) << kPpsPktTypeDescriptions[packet_type] << " packet count : "
       //     << counters_[packet_type].Get();
        counters_[packet_type].Clear();  // ���ͳ����Ϣ�ļ�����
    }
}

}  // namespace BroadCache

