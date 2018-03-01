/*##############################################################################
 * �ļ���   : peer_pool.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : PeerPool��������ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
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
 * ��  ��: �������ڱ��������Ļ�Ծ��peer��Ϣ
 * ��  ��: rosan
 * ʱ  ��: 2013.11.23
 ******************************************************************************/
class PeerPool
{
public:
    explicit PeerPool(boost::asio::io_service& ios, uint32 peer_alive_time = 30 * 60);

    // ��ȡĳ����Դ��Ӧ��peer����Ŀ
    uint32 GetPeerCount(const std::string& info_hash) const;

    // ��ȡ����peer�б�
    std::vector<EndPoint> GetLocalPeers(const std::string& info_hash, uint32 num_want) const; 

    // ����peer��Ϣ
    void AddPeerRequest(const BtGetPeerRequest& request);

private:
    // ���peer�Ƿ�ʱ
    void CheckPeerAlive();

private:
    uint32 peer_alive_time_;  // peer��ʱʱ��
    Timer check_peer_alive_timer_;  // ���ڼ��peer�Ƿ�ʱ�Ķ�ʱ��
    mutable boost::shared_mutex mutex_;  // �������ݵĻ������
    boost::unordered_map<std::string, boost::unordered_map<EndPoint, time_t> > peers_;  // ��������peer�������Ϣ
};

}  // namespace BroadCache

#endif  // HEADER_BT_PEER_POOL
