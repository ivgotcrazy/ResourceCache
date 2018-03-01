/*##############################################################################
 * �ļ���   : tracker_manager.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : TrackerManager���ʵ���ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "tracker_manager.hpp"
#include <iostream>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: ����һ��tracker��Ϣ
 * ��  ��: [in] info_hash tracker��Ӧ����Դ
 *         [in] tracker tracker��������ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void TrackerManager::AddTracker(const std::string& info_hash, const EndPoint& tracker)
{
    trackers_[info_hash].insert(tracker);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡtracker�б�
 * ��  ��: [in] info_hash ��ȡ��info_hash��Ӧ��tracker�б�
 * ����ֵ: tracker�б�
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> TrackerManager::GetTrackers(const std::string& info_hash) const 
{
    auto i = trackers_.find(info_hash);

    return (i == trackers_.end()) ? std::vector<EndPoint>()
        : std::vector<EndPoint>(i->second.begin(), i->second.end());
}

}  // namespace BroadCache
