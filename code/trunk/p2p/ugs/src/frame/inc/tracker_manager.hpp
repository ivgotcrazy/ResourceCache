/*##############################################################################
 * �ļ���   : tracker_manager.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : TrackerManager��������ļ�
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_TRACKER_MANAGER
#define HEADER_TRACKER_MANAGER

#include <vector>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "endpoint.hpp"

namespace BroadCache
{

/*******************************************************************************
 * ��  ��: �������ڱ���ͻ�ȡ��peer�����з��͵�tracker��Ϣ
 * ��  ��: rosan
 * ʱ  ��: 2013.11.23
 ******************************************************************************/
class TrackerManager
{
public:
    // ����һ��tracker��Ϣ
    void AddTracker(const std::string& info_hash, const EndPoint& tracker);

    // ��ȡһ����Դ��Ӧ��tracker�б�
    std::vector<EndPoint> GetTrackers(const std::string& info_hash) const;

private:
    // �����tracker��Ϣ <info-hash, tracker-list>
    boost::unordered_map<std::string, boost::unordered_set<EndPoint> > trackers_;
};

}  // namespace BroadCache

#endif  // HEADER_TRACKER_MANAGER
