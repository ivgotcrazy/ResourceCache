/*##############################################################################
 * �ļ���   : peer_pool.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : PeerPool���ʵ���ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "peer_pool.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: PeerPool��Ĺ��캯��
 * ��  ��: [in][out] ios IO�������
 *         [in] peer_alive_time ����peer���ʱ�䣨��λ���룩
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
PeerPool::PeerPool(boost::asio::io_service& ios, uint32 peer_alive_time)
    : peer_alive_time_(peer_alive_time),
      check_peer_alive_timer_(ios, boost::bind(&PeerPool::CheckPeerAlive, this), 1)
{
    check_peer_alive_timer_.Start();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡĳ����Դ��Ӧ������peer��Ŀ
 * ��  ��: [in] info_hash ��ʶĳ����Դ��info-hashֵ
 * ����ֵ: ����Դ��Ӧ������peer��Ŀ
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
uint32 PeerPool::GetPeerCount(const std::string& info_hash) const
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);

    auto i = peers_.find(info_hash);
    return (i != peers_.end()) ? i->second.size() : 0;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡĳ����Դ��Ӧ������peer
 * ��  ��: [in] info_hash ����Դ��Ӧ��info-hashֵ
 *         [in] num_want �������ص�����peer��Ŀ
 * ����ֵ: ����Դ��Ӧ������peer
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::vector<EndPoint> PeerPool::GetLocalPeers(const std::string& info_hash,
                                                uint32 num_want) const
{
    std::vector<EndPoint> peers;  // ���ص�����peer
    boost::lock_guard<decltype(mutex_)> guard(mutex_);

    auto i = peers_.find(info_hash);
    if (i != peers_.end())
    {
        // ���ѡ�����num_want������peer
        auto selected_peers = RandomSelect(i->second.begin(), i->second.end(), num_want);
        FOREACH(e, selected_peers)
        {
            peers.push_back(e->first);
        }
    }

    return peers;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��������peer��Ϣ
 * ��  ��: [in] request ����peer�����Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PeerPool::AddPeerRequest(const BtGetPeerRequest& request)
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);
    peers_[request.info_hash][request.requester] = request.time; 
}

/*------------------------------------------------------------------------------
 * ��  ��: �������peer�Ƿ�ʱ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PeerPool::CheckPeerAlive()
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);

    time_t now = time(nullptr);  // ��ǰʱ��
    for (auto i = peers_.begin(); i != peers_.end(); )
    {
        auto& peers = i->second;  // ĳ����Դ��Ӧ������peer
        for (auto j = peers.begin(); j != peers.end(); )
        {
            bool timeout = (now > j->second + peer_alive_time_);  // �Ƿ�ʱ
            j = (timeout ? Erase(peers, j) : ++j); 
        }
        
        i = (peers.empty() ? Erase(peers_, i) : ++i);
    }
}

}  // namespace BroadCache
