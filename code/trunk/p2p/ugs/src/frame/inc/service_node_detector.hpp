/*##############################################################################
 * �ļ���   : service_node_detector.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.15
 * �ļ����� : ��ServiceNodeDetector�������ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_SERVICE_NODE_DETECTOR
#define HEADER_SERVICE_NODE_DETECTOR

#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "timer.hpp"

namespace BroadCache
{

/*******************************************************************************
 *��  ��: ���ڼ��RCS���״̬
 *��  ��: rosan
 *ʱ  ��: 2013.11.15
 ******************************************************************************/
class ServiceNodeDetector
{
public:
    typedef boost::function<void(const EndPoint&)> TimeoutHandler;  // ��⵽RCS��ʱ

public:
    ServiceNodeDetector(boost::asio::io_service& ios);

    // �յ�keep-alive��Ϣ
    void OnKeepAliveMsg(const EndPoint& endpoint);

    // ��ȡRCS��ʱ�Ļص�����
    TimeoutHandler GetTimeoutHandler() const;

    // ����RCS��ʱ�Ļص�����
    void SetTimeoutHandler(const TimeoutHandler& handler);

private:
    // ��ʱ���ص�����
    void OnTimer();

private:
    static const uint32 kTimerInterval = 3;  // ��ʱ��ʱ��������λΪ�룩
    static const uint32 kTimeoutMaxTimes = 3;  // RCS��ʱ�������������˴�������ΪRCS���ߣ�

    Timer timer_;  // ���RCS���״̬�Ķ�ʱ��
    TimeoutHandler timeout_handler_;  // ��⵽RCS��ʱ
    boost::unordered_map<EndPoint, uint32> timeout_times_;  // RCS��ʱ����
};

}  // namespace BroadCache

#endif  // HEADER_SERVICE_NODE_DETECTOR 
