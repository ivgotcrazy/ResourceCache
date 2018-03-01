/*#############################################################################
 * �ļ���   : policy.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��14��
 * �ļ����� : Policyʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "policy.hpp"
#include "rcs_util.hpp"
#include "peer_connection.hpp"
#include "session.hpp"
#include "ip_filter.hpp"
#include "rcs_config_parser.hpp"

namespace BroadCache
{

// ��ȡ��/����peerʱ����(��)
static const uint32 kRetrivePeersInterval = 180;

// ��ʼʹ��inner proxy��ʱ��(��)
static const uint32 kRetriveInnerProxyTime = 30;

// ��ʼʹ��outer proxy��ʱ��(��)
static const uint32 kRetriveOuterProxyInterval = 300;

// ����peerʧ�ܺ���������peerʱ����(��)
static const uint32 kReConnectPeerInterval = 60;

// ����peerʧ�ܺ���������peer����
static const uint32 kReConnectPeerTimes = 3;


/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Policy::Policy(const TorrentSP& torrent) 
	: torrent_(torrent)
	, blocked_for_monitor_(false)
	, blocked_for_proxy_(false)
	, retrived_cached_rcses_(false)
	, retrived_inner_proxies_(false)
	, retrived_outer_proxies_(false)
{
	// ��һ�λ�ȡpeerӦ�þ�����У����ｫ�ϴλ�ȡpeerʱ������Ϊһ����Сֵ
	last_retrive_peers_time_ = ptime(boost::gregorian::date(1900, boost::gregorian::Jan, 1));

	GET_RCS_CONFIG_INT("bt.distributed-download.download-speed-threshold", use_proxy_download_speed_);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��11��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Policy::~Policy()
{
	LOG(INFO) << ">>> Destructing policy";
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ���ִ���
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::SetSeed()
{
	ClearOuterPeers();

	ClearOuterConnections();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������ʧ�ܵ�peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::UpdateFailedPeers()
{
	boost::mutex::scoped_lock lock(peer_mutex_);

	PeerMapIter iter = failed_peers_.begin();
	for ( ; iter != failed_peers_.end(); )
	{
		int64 duration = GetDurationSeconds(iter->second.last_connect, TimeNow());
		BC_ASSERT(duration >= 0);

		int64 next_connect_interval = pow(2, iter->second.tried_times) * kReConnectPeerInterval;

		if (duration < next_connect_interval)
		{
			++iter;
			continue;
		}

		if (iter->second.peer_type == PEER_INNER)
		{
			candidate_inner_peers_.insert(iter->second);
		}
		else if (iter->second.peer_type == PEER_OUTER)
		{
			candidate_outer_peers_.insert(iter->second);
		}
		else
		{
			//TODO:
		}

		iter = failed_peers_.erase(iter);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����������ӵĶ�ʱ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::CallConnectionTickProc()
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	for (decltype(*peer_conn_map_.begin())& peer_item : peer_conn_map_)
	{
		if(peer_item.second)
		{
			peer_item.second->OnTick(); 
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::OnTick()
{
	ClearInactiveConnections();
	
	CallConnectionTickProc();
		
	if (!IsBlocked())
	{
		UpdateFailedPeers();

		TryRetrieveConnectablePeers();

		TryConnectPeers();
	}

	LOG(WARNING) << "Peer Connection num: " << peer_conn_map_.size();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ի�ȡinner proxy
 * ��  ��: [in] torrent Torrent
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryRetrieveConnectablePeers()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	TryRetrivePeers(torrent);

	TryRetriveCachedRCSes(torrent);

	TryRetriveInnerProxies(torrent);

	TryRetriveOuterProxies(torrent);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryConnectPeers()
{
	TryConnectInnerPeers();

	TryConnectOuterPeers();

	TryConnectCachedRCSes();

	TryConnectInnerProxies();

	TryConnectOuterProxies();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ի�ȡ������peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryRetrivePeers(const TorrentSP& torrent)
{
	if (!candidate_inner_peers_.empty()) return;

	uint64 seconds = GetDurationSeconds(last_retrive_peers_time_, TimeNow());
	if (seconds < kRetrivePeersInterval) return;
	
	torrent->RetrivePeers();

	last_retrive_peers_time_ = TimeNow();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ի�ȡinner proxy
 * ��  ��: [in] torrent Torrent
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryRetriveInnerProxies(const TorrentSP& torrent)
{
	if (torrent->is_seed()) return;

	// inner proxyֻ�ܻ�ȡһ��
	if (retrived_inner_proxies_) return;

	if (torrent->alive_time() < kRetriveInnerProxyTime) return;

	uint32 alive_time = torrent->alive_time();

	// ���ж���torrent�������ع����ж�������
	if (GetDownloadTotalSize() / alive_time > use_proxy_download_speed_) return;

	torrent->RetriveInnerProxies();

	retrive_inner_proxy_time_ = TimeNow();

	retrived_inner_proxies_ = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ի�ȡouter proxy
 * ��  ��: [in] torrent Torrent
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryRetriveOuterProxies(const TorrentSP& torrent)
{
	if (torrent->is_seed()) return;
	
	if (!retrived_inner_proxies_) return; // ����ʹ��inner proxy

	if (retrived_outer_proxies_) return; // outer proxyֻ�ܻ�ȡһ��

	// ��ȡinner proxy����Ҫ��һ��ʱ�䣬���ܻ�ȡouter proxy
	int64 duration = GetDurationSeconds(retrive_inner_proxy_time_, TimeNow());
	BC_ASSERT(duration >= 0);
	if (static_cast<uint32>(duration) < kRetriveOuterProxyInterval) return;

	// ���ж���torrent�������ع����ж�������
	uint32 alive_time = torrent->alive_time();
	if (GetDownloadTotalSize() / alive_time > use_proxy_download_speed_) return;

	torrent->RetriveOuterProxies();

	retrived_outer_proxies_ = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ի�ȡcahced RCS
 * ��  ��: [in] torrent Torrent
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryRetriveCachedRCSes(const TorrentSP& torrent)
{
	if (torrent->is_seed()) return;

	if (retrived_cached_rcses_) return; // cached rcsֻ��ȡһ��

	torrent->RetriveCachedRCSes();

	retrived_cached_rcses_ = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƿ���Ҫ���Ӹ���peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Policy::IfNeedMoreInnerPeers()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Policy::IfNeedMoreOuterPeers()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	if (torrent->is_seed()) return false;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryConnectInnerPeers()
{
	if (!IfNeedMoreInnerPeers()) return;

	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_inner_peers_, peer)) return;
	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryConnectOuterPeers()
{
	if (!IfNeedMoreOuterPeers()) return;

	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_outer_peers_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������cached RCS
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryConnectCachedRCSes()
{
	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_cached_rcses_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������inner proxy
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryConnectInnerProxies()
{
	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_inner_proxies_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������outer proxy
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::TryConnectOuterProxies()
{
	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_outer_proxies_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������ӳɹ�
 * ��  ��: [in] peer peer
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::PeerConnected(PeerEntry peer, const PeerConnectionSP& conn)
{
	peer.conn = conn;
	peer.last_connect = TimeNow();

	boost::mutex::scoped_lock lock(peer_mutex_);

	connected_peers_.insert(PeerMap::value_type(peer.address, peer));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������peerʧ��
 * ��  ��: [in] peer ���ӵ�peer
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::FailToConnectPeer(PeerEntry peer, const PeerConnectionSP& peer_conn)
{
	if (peer.tried_times >= kReConnectPeerTimes) return;

	peer.last_connect = TimeNow();
	peer.tried_times++;

	boost::mutex::scoped_lock lock(peer_mutex_);

	peer_conn_map_.erase(peer_conn->connection_id());

	failed_peers_.insert(PeerMap::value_type(peer.address, peer));
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������Ӵ���
 * ��  ��: [in] peer PeerEntry
 *         [in] peer_conn �����ɹ���PeerConnection
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��04��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::OnActiveConnectionConnected(ConnectionError error, PeerEntry peer, 
	                                     const PeerConnectionWP& weak_conn)
{
	PeerConnectionSP peer_conn = weak_conn.lock();
	if (!peer_conn) return;

	if (error != CONN_ERR_SUCCESS)
	{
		FailToConnectPeer(peer, peer_conn);

		LOG(ERROR) << "Fail to connect peer | " << peer_conn;
		return;
	}

	LOG(INFO) << "Active connection connected | " << peer_conn;

	PeerConnected(peer, peer_conn); // ���ӳɹ������뵽����������

	if (!peer_conn->Start())		// ����Э��״̬��
	{
		LOG(ERROR) << "Fail to start peer connection" << peer_conn;
		return;
	}

	LOG(INFO) << "Success to connect peer | " << peer_conn;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������һ��Peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::ConnectOnePeer(const PeerEntry& peer)
{
	BC_ASSERT(!peer.conn);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	PeerConnectionSP peer_conn = torrent->session().CreatePeerConnection(
									peer.address, peer.peer_type);
	if (!peer_conn)
	{
		LOG(ERROR) << "Fail to create peer connection";
		return;
	}

	peer_conn->AttachTorrent(torrent); // ��������ֱ�ӹ���torrent

	peer_conn->Init(); // ��ʼ������

	AddPeerConnection(peer_conn); // �����Ӽ����������

	// Ϊ�˱���ѭ�����ã�����ʹ��weak_ptr
	PeerConnectionWP weak_conn = peer_conn;
	peer_conn->socket_connection()->set_connect_handler(
		boost::bind(&Policy::OnActiveConnectionConnected, 
		            shared_from_this(), _1, peer, weak_conn));
	
	// ������peer��������
	peer_conn->socket_connection()->Connect();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���peer
 * ��  ��: [in] peer peer
 *         [in] type peer����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::AddPeer(const EndPoint& peer, PeerType peer_type, PeerRetriveType retrive_type)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// ����Ѿ����֣���ֻ��������peer
	if (torrent->is_seed() && peer_type != PEER_INNER) return;

	boost::mutex::scoped_lock lock(peer_mutex_);

	// �Ѿ��������������������
	if (connected_peers_.end() != connected_peers_.find(peer)) return;

	// �Ѿ���ʧ�ܶ���Ҳ�����������
	if (failed_peers_.end() != failed_peers_.find(peer)) return;

	PeerEntry peer_entry(peer, peer_type, retrive_type);

	switch (peer_type)
	{
	case PEER_INNER:
		candidate_inner_peers_.insert(peer_entry);
		break;

	case PEER_OUTER:
		candidate_outer_peers_.insert(peer_entry);
		break;

	case PEER_CACHED_RCS:
		candidate_cached_rcses_.insert(peer_entry);
		break;

	case PEER_INNER_PROXY:
		candidate_inner_proxies_.insert(peer_entry);
		break;

	case PEER_OUTER_PROXY:
		candidate_outer_proxies_.insert(peer_entry);

	default:
		BC_ASSERT(false);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�����ӵ�peer
 * ��  ��: [out] peer peer
 * ����ֵ: �Ƿ���ڿ�����peer
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Policy::GetOneCandidatePeer(PeerSet& peers, PeerEntry& peer)
{
	boost::mutex::scoped_lock lock(peer_mutex_);

	if (peers.empty()) return false;

	PeerSetIter iter = peers.begin();

	peer = *(iter);

	peers.erase(iter);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ӹرմ���
 * ��  ��: [in] peer peer
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ��?teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::ConnectionClosed(const PeerConnectionSP& conn)
{
	// ֻ������������
	if (conn->socket_connection()->connection_type() != CONN_ACTIVE) return;

	boost::mutex::scoped_lock lock(peer_mutex_);

	PeerMapIter iter = connected_peers_.find(conn->connection_id().remote);
	if (iter == connected_peers_.end())
	{
		LOG(WARNING) << "Can't find connection | " << conn->connection_id().remote;
		return;
	}

	connected_peers_.erase(iter);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������peer
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::ClearOuterPeers()
{
	boost::mutex::scoped_lock lock(peer_mutex_);
	
	candidate_outer_peers_.clear();

	PeerMapIter iter = failed_peers_.begin();
	for ( ; iter != failed_peers_.end(); )
	{
		if (!iter->second.peer_type == PEER_INNER)
		{
			iter = failed_peers_.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����������
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::ClearOuterConnections()
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	PeerConnMapIter iter = peer_conn_map_.begin();
	for ( ; iter != peer_conn_map_.end(); )
	{
		if (!iter->second->peer_type() == PEER_INNER)
		{
			ConnectionClosed(iter->second);

			LOG(INFO) << "Removed one connection from torrent | " << iter->first;

			iter = peer_conn_map_.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�������Ӹ���
 * ��  ��: 
 * ����ֵ: �������Ӹ���
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Policy::GetPassivePeerConnNum()
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	uint64 passive_conn_num = 0;

	FOREACH(element, peer_conn_map_)
	{
		SocketConnectionSP sock_conn = element.second->socket_connection();
		ConnectionType conn_type = sock_conn->connection_type();

		if (CONN_PASSIVE == conn_type) passive_conn_num++;
	}

	return passive_conn_num;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ������������
 * ��  ��: 
 * ����ֵ: ���Ӹ���
 * ��  ��:
 *   ʱ�� 2013��12��03��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Policy::GetPeerConnNum()
{ 
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	return peer_conn_map_.size(); 
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�������Ӹ���
 * ��  ��: 
 * ����ֵ: �������Ӹ���
 * ��  ��:
 *   ʱ�� 2013��12��03��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint32 Policy::GetProxyPeerConnNum()
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	uint64 proxy_conn_num = 0;

	FOREACH(element, peer_conn_map_)
	{
		if (element.second->peer_conn_type() == PEER_CONN_PROXY)
		{
			proxy_conn_num++;
		}		
	}

	return proxy_conn_num;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���PeerConnection
 * ��  ��: [in] conn ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::AddPeerConnection(const PeerConnectionSP& conn)
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	peer_conn_map_.insert(PeerConnMap::value_type(conn->connection_id(), conn));
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������Ӵ���
 * ��  ��: [in] conn ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::IncommingPeerConnecton(const PeerConnectionSP& conn)
{
	LOG(INFO) << "Incomming a new peer connection";

	if (IsBlocked()) return;

	AddPeerConnection(conn);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Ѿ��رյ�����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::ClearInactiveConnections()
{
	//TODO: ������������϶���Ӱ�����ܵģ������Ż�
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	uint32 erase_conn_num = 0;

	PeerConnMapIter iter = peer_conn_map_.begin();
	for ( ; iter != peer_conn_map_.end(); )
	{
		if (!iter->second->IsClosed() && !iter->second->IsInactive())
		{
			++iter;
			continue;
		}

		ConnectionClosed(iter->second);
		iter = peer_conn_map_.erase(iter);
		
		erase_conn_num++;
	}	

	if (erase_conn_num != 0)
	{
		LOG(INFO) << "Removed " << erase_conn_num << " connection from torrent";

		ProxyProcForRemovedConn();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ɾ�����Ӻ�Ĵ�����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::ProxyProcForRemovedConn()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	if (!torrent->is_proxy()) return;

	bool have_proxy_conn = false;

	PeerConnMapIter iter = peer_conn_map_.begin();
	for ( ; iter != peer_conn_map_.end(); )
	{
		PeerConnType conn_type = iter->second->peer_conn_type();
		if (!iter->second->IsClosed() && (conn_type == PEER_CONN_PROXY))
		{
			have_proxy_conn = true;
			break;
		}
	}

	if (!have_proxy_conn)
	{
		peer_conn_map_.clear();
		blocked_for_proxy_ = true;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ݹ���ѡȡ����
 * ����ֵ: [in] handler ѡ�����
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
std::vector<PeerConnectionSP> Policy::SelectConnection(SelectHandler handler)
{
	std::vector<PeerConnectionSP> selected_conns;

	PeerConnMapIter it = peer_conn_map_.begin();
	for ( ; it != peer_conn_map_.end(); it++)
	{
		if (handler(it->second))
		{
			selected_conns.push_back(it->second);
		}
	}

	return selected_conns;
}

/*------------------------------------------------------------------------------
 * ��  ��: ֪ͨ������peer�Լ��Ѿ�ӵ����һ��piece
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
void Policy::HavePiece(uint32 piece)
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);
	PeerConnMapIter iter = peer_conn_map_.begin();
	for ( ; iter != peer_conn_map_.end(); ++iter)
	{
		iter->second->HavePiece(piece);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��:��ȡһ��torrent�ϴ��ܴ�С
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��1��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Policy::GetUploadTotalSize()
{	
	uint64 total_upload = 0;
	PeerConnMapIter it = peer_conn_map_.begin();
	for(; it != peer_conn_map_.end(); ++ it)
	{
		total_upload += it->second->GetUploadSize();
	}
	return total_upload;
}

/*-----------------------------------------------------------------------------
 * ��  ��:��ȡһ��torrent�����ܴ�С
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��1��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Policy::GetDownloadTotalSize()
{	
	uint64 total_download = 0;
	PeerConnMapIter it = peer_conn_map_.begin();
	for(; it != peer_conn_map_.end(); ++ it)
	{
		total_download += it->second->GetDownloadSize();
	}
	return total_download;
}

/*-----------------------------------------------------------------------------
 * ��  ��:��ȡ�ϴ�����
 * ��  ��: 
 * ����ֵ: uint64 ����ֵ
 * ��  ��:
 *   ʱ�� 2013��11��1��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Policy::GetUploadBandwidth()
{	
	uint64 upload_bandwidth = 0;

	PeerConnMapIter it = peer_conn_map_.begin();
	for(; it != peer_conn_map_.end(); ++ it)
	{
		upload_bandwidth += it->second->GetUploadBandwidth();
	}
	return upload_bandwidth;
}

/*-----------------------------------------------------------------------------
 * ��  ��:��ȡ���ش���
 * ��  ��: 
 * ����ֵ: uint64 ����ֵ
 * ��  ��:
 *   ʱ�� 2013��11��1��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Policy::GetDownloadBandwidth()
{	
	uint64 download_bandwidth = 0;

	PeerConnMapIter it = peer_conn_map_.begin();
	for(; it != peer_conn_map_.end(); ++ it)
	{
		download_bandwidth += it->second->GetDownloadBandwidth();
	}
	return download_bandwidth;
} 

/*-----------------------------------------------------------------------------
 * ��  ��:��ȡ����torrent�µ�inner_peer��outer_peer����Ŀ 
 * ��  ��: [out]   inner_peer_num
 		   [out]   outer_peer_num
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��1��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Policy::GetInOutPeerNum(uint32 * inner_peer_num,uint32 * outer_peer_num)
{
	PeerConnMapIter it = peer_conn_map_.begin();
	for(; it != peer_conn_map_.end(); ++ it)
	{
		if(it->second->peer_type() == PEER_INNER)(*inner_peer_num)++;      
 	    else (*outer_peer_num)++;
	}		
}

}

