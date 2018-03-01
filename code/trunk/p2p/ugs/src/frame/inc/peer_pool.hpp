/*##############################################################################
 * 文件名   : peer_pool.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : PeerPool类的声明文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_BT_PEER_POOL
#define HEADER_BT_PEER_POOL

#include "depend.hpp"
#include "timer.hpp"
#include "endpoint.hpp"
#include "bt_types.hpp"

#include <boost/thread.hpp>

namespace BroadCache
{

/*******************************************************************************
 * 描  述: 此类用于保存内网的活跃的peer信息
 * 作  者: rosan
 * 时  间: 2013.11.23
 ******************************************************************************/
class PeerPool
{
public:
    explicit PeerPool(boost::asio::io_service& ios, uint32 peer_alive_time = 30 * 60);

    // 获取某个资源对应的peer的数目
    uint32 GetPeerCount(const std::string& info_hash) const;

    // 获取内网peer列表
    std::vector<EndPoint> GetLocalPeers(const std::string& info_hash, uint32 num_want) const; 

    // 增加peer信息
    void AddPeerRequest(const BtGetPeerRequest& request);

private:
    // 检查peer是否超时
    void CheckPeerAlive();

private:
    uint32 peer_alive_time_;  // peer超时时间
    Timer check_peer_alive_timer_;  // 用于检查peer是否超时的定时器
    mutable boost::shared_mutex mutex_;  // 控制数据的互斥访问
    boost::unordered_map<std::string, boost::unordered_map<EndPoint, time_t> > peers_;  // 保存内网peer的相关信息
};

}  // namespace BroadCache

#endif  // HEADER_BT_PEER_POOL
