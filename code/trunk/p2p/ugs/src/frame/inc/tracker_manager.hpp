/*##############################################################################
 * 文件名   : tracker_manager.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : TrackerManager类的声明文件
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
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
 * 描  述: 此类用于保存和获取从peer请求中发送的tracker信息
 * 作  者: rosan
 * 时  间: 2013.11.23
 ******************************************************************************/
class TrackerManager
{
public:
    // 增加一条tracker信息
    void AddTracker(const std::string& info_hash, const EndPoint& tracker);

    // 获取一个资源对应的tracker列表
    std::vector<EndPoint> GetTrackers(const std::string& info_hash) const;

private:
    // 保存的tracker信息 <info-hash, tracker-list>
    boost::unordered_map<std::string, boost::unordered_set<EndPoint> > trackers_;
};

}  // namespace BroadCache

#endif  // HEADER_TRACKER_MANAGER
