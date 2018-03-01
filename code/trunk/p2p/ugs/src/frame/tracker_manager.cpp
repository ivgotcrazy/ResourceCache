/*##############################################################################
 * 文件名   : tracker_manager.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : TrackerManager类的实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "tracker_manager.hpp"
#include <iostream>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 增加一条tracker信息
 * 参  数: [in] info_hash tracker对应的资源
 *         [in] tracker tracker服务器地址
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void TrackerManager::AddTracker(const std::string& info_hash, const EndPoint& tracker)
{
    trackers_[info_hash].insert(tracker);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取tracker列表
 * 参  数: [in] info_hash 获取此info_hash对应的tracker列表
 * 返回值: tracker列表
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> TrackerManager::GetTrackers(const std::string& info_hash) const 
{
    auto i = trackers_.find(info_hash);

    return (i == trackers_.end()) ? std::vector<EndPoint>()
        : std::vector<EndPoint>(i->second.begin(), i->second.end());
}

}  // namespace BroadCache
