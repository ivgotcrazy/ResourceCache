/*##############################################################################
 * 文件名   : peer_pool.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : PeerPool类的实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "peer_pool.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: PeerPool类的构造函数
 * 参  数: [in][out] ios IO服务对象
 *         [in] peer_alive_time 内网peer存活时间（单位：秒）
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
PeerPool::PeerPool(boost::asio::io_service& ios, uint32 peer_alive_time)
    : peer_alive_time_(peer_alive_time),
      check_peer_alive_timer_(ios, boost::bind(&PeerPool::CheckPeerAlive, this), 1)
{
    check_peer_alive_timer_.Start();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取某个资源对应的内网peer数目
 * 参  数: [in] info_hash 标识某个资源的info-hash值
 * 返回值: 此资源对应的内网peer数目
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
uint32 PeerPool::GetPeerCount(const std::string& info_hash) const
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);

    auto i = peers_.find(info_hash);
    return (i != peers_.end()) ? i->second.size() : 0;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取某个资源对应的内网peer
 * 参  数: [in] info_hash 此资源对应的info-hash值
 *         [in] num_want 期望返回的内网peer数目
 * 返回值: 此资源对应的内网peer
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> PeerPool::GetLocalPeers(const std::string& info_hash,
                                                uint32 num_want) const
{
    std::vector<EndPoint> peers;  // 返回的内网peer
    boost::lock_guard<decltype(mutex_)> guard(mutex_);

    auto i = peers_.find(info_hash);
    if (i != peers_.end())
    {
        // 随机选择最多num_want个内网peer
        auto selected_peers = RandomSelect(i->second.begin(), i->second.end(), num_want);
        FOREACH(e, selected_peers)
        {
            peers.push_back(e->first);
        }
    }

    return peers;
}

/*------------------------------------------------------------------------------
 * 描  述: 增加内网peer信息
 * 参  数: [in] request 内网peer相关信息
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PeerPool::AddPeerRequest(const BtGetPeerRequest& request)
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);
    peers_[request.info_hash][request.requester] = request.time; 
}

/*------------------------------------------------------------------------------
 * 描  述: 检测内网peer是否超时
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PeerPool::CheckPeerAlive()
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);

    time_t now = time(nullptr);  // 当前时间
    for (auto i = peers_.begin(); i != peers_.end(); )
    {
        auto& peers = i->second;  // 某个资源对应的内网peer
        for (auto j = peers.begin(); j != peers.end(); )
        {
            bool timeout = (now > j->second + peer_alive_time_);  // 是否超时
            j = (timeout ? Erase(peers, j) : ++j); 
        }
        
        i = (peers.empty() ? Erase(peers_, i) : ++i);
    }
}

}  // namespace BroadCache
