/*##############################################################################
 * �ļ���   : service_node_detector.cpp 
 * ������   : rosan 
 * ����ʱ�� : 2013.11.16
 * �ļ����� : ��ServiceNodeDetector��ʵ���� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "service_node_detector.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: ServiceNodeDetector��Ĺ��캯��
 * ��  ��: [out] ios IO�������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
ServiceNodeDetector::ServiceNodeDetector(boost::asio::io_service& ios) : 
    timer_(ios, boost::bind(&ServiceNodeDetector::OnTimer, this), kTimerInterval)
{
    timer_.Start();
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�keep-alive��Ϣ
 * ��  ��: [in] endpoint RCS��ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void ServiceNodeDetector::OnKeepAliveMsg(const EndPoint& endpoint)
{
    timeout_times_[endpoint] = 0;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡRCS��ʱ�Ļص�����
 * ��  ��: 
 * ����ֵ: RCS��ʱ�ص�����
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
ServiceNodeDetector::TimeoutHandler ServiceNodeDetector::GetTimeoutHandler() const
{
    return timeout_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����RCS��ʱ�Ļص�����
 * ��  ��: [in] handler RCS��ʱ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void ServiceNodeDetector::SetTimeoutHandler(const TimeoutHandler& handler)
{
    timeout_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ʱ���ص�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
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
