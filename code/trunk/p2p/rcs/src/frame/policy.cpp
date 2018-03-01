/*#############################################################################
 * 文件名   : policy.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月14日
 * 文件描述 : Policy实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "policy.hpp"
#include "rcs_util.hpp"
#include "peer_connection.hpp"
#include "session.hpp"
#include "ip_filter.hpp"
#include "rcs_config_parser.hpp"

namespace BroadCache
{

// 获取内/外网peer时间间隔(秒)
static const uint32 kRetrivePeersInterval = 180;

// 开始使用inner proxy的时间(秒)
static const uint32 kRetriveInnerProxyTime = 30;

// 开始使用outer proxy的时间(秒)
static const uint32 kRetriveOuterProxyInterval = 300;

// 连接peer失败后，重新连接peer时间间隔(秒)
static const uint32 kReConnectPeerInterval = 60;

// 连接peer失败后，重新连接peer次数
static const uint32 kReConnectPeerTimes = 3;


/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Policy::Policy(const TorrentSP& torrent) 
	: torrent_(torrent)
	, blocked_for_monitor_(false)
	, blocked_for_proxy_(false)
	, retrived_cached_rcses_(false)
	, retrived_inner_proxies_(false)
	, retrived_outer_proxies_(false)
{
	// 第一次获取peer应该尽快进行，这里将上次获取peer时间设置为一个很小值
	last_retrive_peers_time_ = ptime(boost::gregorian::date(1900, boost::gregorian::Jan, 1));

	GET_RCS_CONFIG_INT("bt.distributed-download.download-speed-threshold", use_proxy_download_speed_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月11日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Policy::~Policy()
{
	LOG(INFO) << ">>> Destructing policy";
}

/*-----------------------------------------------------------------------------
 * 描  述: 开始做种处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::SetSeed()
{
	ClearOuterPeers();

	ClearOuterConnections();
}

/*-----------------------------------------------------------------------------
 * 描  述: 更新连接失败的peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 调用所有连接的定时器处理函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月10日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 定时器处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 尝试获取inner proxy
 * 参  数: [in] torrent Torrent
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 尝试连接peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 尝试获取可连接peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 尝试获取inner proxy
 * 参  数: [in] torrent Torrent
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryRetriveInnerProxies(const TorrentSP& torrent)
{
	if (torrent->is_seed()) return;

	// inner proxy只能获取一次
	if (retrived_inner_proxies_) return;

	if (torrent->alive_time() < kRetriveInnerProxyTime) return;

	uint32 alive_time = torrent->alive_time();

	// 此判断在torrent整个下载过程中都起作用
	if (GetDownloadTotalSize() / alive_time > use_proxy_download_speed_) return;

	torrent->RetriveInnerProxies();

	retrive_inner_proxy_time_ = TimeNow();

	retrived_inner_proxies_ = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试获取outer proxy
 * 参  数: [in] torrent Torrent
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryRetriveOuterProxies(const TorrentSP& torrent)
{
	if (torrent->is_seed()) return;
	
	if (!retrived_inner_proxies_) return; // 优先使用inner proxy

	if (retrived_outer_proxies_) return; // outer proxy只能获取一次

	// 获取inner proxy后，需要等一段时间，才能获取outer proxy
	int64 duration = GetDurationSeconds(retrive_inner_proxy_time_, TimeNow());
	BC_ASSERT(duration >= 0);
	if (static_cast<uint32>(duration) < kRetriveOuterProxyInterval) return;

	// 此判断在torrent整个下载过程中都起作用
	uint32 alive_time = torrent->alive_time();
	if (GetDownloadTotalSize() / alive_time > use_proxy_download_speed_) return;

	torrent->RetriveOuterProxies();

	retrived_outer_proxies_ = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试获取cahced RCS
 * 参  数: [in] torrent Torrent
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryRetriveCachedRCSes(const TorrentSP& torrent)
{
	if (torrent->is_seed()) return;

	if (retrived_cached_rcses_) return; // cached rcs只获取一次

	torrent->RetriveCachedRCSes();

	retrived_cached_rcses_ = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 是否需要连接更多peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Policy::IfNeedMoreInnerPeers()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试连接内网peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Policy::IfNeedMoreOuterPeers()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	if (torrent->is_seed()) return false;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试连接内网peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryConnectInnerPeers()
{
	if (!IfNeedMoreInnerPeers()) return;

	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_inner_peers_, peer)) return;
	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试连接外网peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryConnectOuterPeers()
{
	if (!IfNeedMoreOuterPeers()) return;

	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_outer_peers_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试连接cached RCS
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryConnectCachedRCSes()
{
	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_cached_rcses_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试连接inner proxy
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryConnectInnerProxies()
{
	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_inner_proxies_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * 描  述: 尝试连接outer proxy
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::TryConnectOuterProxies()
{
	PeerEntry peer;
	if (!GetOneCandidatePeer(candidate_outer_proxies_, peer)) return;

	ConnectOnePeer(peer);
}

/*-----------------------------------------------------------------------------
 * 描  述: 主动连接成功
 * 参  数: [in] peer peer
 * 返回值: 
 * 修  改:
 *   时间 2013年10月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::PeerConnected(PeerEntry peer, const PeerConnectionSP& conn)
{
	peer.conn = conn;
	peer.last_connect = TimeNow();

	boost::mutex::scoped_lock lock(peer_mutex_);

	connected_peers_.insert(PeerMap::value_type(peer.address, peer));
}

/*-----------------------------------------------------------------------------
 * 描  述: 主动连接peer失败
 * 参  数: [in] peer 连接的peer
 * 返回值: 
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 主动连接处理
 * 参  数: [in] peer PeerEntry
 *         [in] peer_conn 创建成功的PeerConnection
 * 返回值:
 * 修  改:
 *   时间 2013年11月04日
 *   作者 teck_zhou
 *   描述 创建
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

	PeerConnected(peer, peer_conn); // 连接成功，加入到已连接容器

	if (!peer_conn->Start())		// 启动协议状态机
	{
		LOG(ERROR) << "Fail to start peer connection" << peer_conn;
		return;
	}

	LOG(INFO) << "Success to connect peer | " << peer_conn;
}

/*-----------------------------------------------------------------------------
 * 描  述: 主动连接一个Peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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

	peer_conn->AttachTorrent(torrent); // 主动连接直接关联torrent

	peer_conn->Init(); // 初始化连接

	AddPeerConnection(peer_conn); // 将连接加入管理容器

	// 为了避免循环引用，这里使用weak_ptr
	PeerConnectionWP weak_conn = peer_conn;
	peer_conn->socket_connection()->set_connect_handler(
		boost::bind(&Policy::OnActiveConnectionConnected, 
		            shared_from_this(), _1, peer, weak_conn));
	
	// 主动向peer发起连接
	peer_conn->socket_connection()->Connect();
}

/*-----------------------------------------------------------------------------
 * 描  述: 添加peer
 * 参  数: [in] peer peer
 *         [in] type peer类型
 * 返回值:
 * 修  改:
 *   时间 2013年10月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::AddPeer(const EndPoint& peer, PeerType peer_type, PeerRetriveType retrive_type)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// 如果已经做种，则只连接内网peer
	if (torrent->is_seed() && peer_type != PEER_INNER) return;

	boost::mutex::scoped_lock lock(peer_mutex_);

	// 已经建立连接则不用再添加了
	if (connected_peers_.end() != connected_peers_.find(peer)) return;

	// 已经在失败队列也不用再添加了
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
 * 描  述: 获取可连接的peer
 * 参  数: [out] peer peer
 * 返回值: 是否存在可连接peer
 * 修  改:
 *   时间 2013年10月14日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 连接关闭处理
 * 参  数: [in] peer peer
 * 返回值: 
 * 修  改:
 *   时间 2013年10月14日
 *   髡?teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::ConnectionClosed(const PeerConnectionSP& conn)
{
	// 只处理主动连接
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
 * 描  述: 清除外网peer
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 清除外网连接
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 获取被动连接个数
 * 参  数: 
 * 返回值: 被动连接个数
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 获取所有连接数量
 * 参  数: 
 * 返回值: 连接个数
 * 修  改:
 *   时间 2013年12月03日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Policy::GetPeerConnNum()
{ 
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	return peer_conn_map_.size(); 
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取代理连接个数
 * 参  数: 
 * 返回值: 代理连接个数
 * 修  改:
 *   时间 2013年12月03日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 添加PeerConnection
 * 参  数: [in] conn 连接
 * 返回值: 
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::AddPeerConnection(const PeerConnectionSP& conn)
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	peer_conn_map_.insert(PeerConnMap::value_type(conn->connection_id(), conn));
}

/*-----------------------------------------------------------------------------
 * 描  述: 被动连接处理
 * 参  数: [in] conn 连接
 * 返回值: 
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::IncommingPeerConnecton(const PeerConnectionSP& conn)
{
	LOG(INFO) << "Incomming a new peer connection";

	if (IsBlocked()) return;

	AddPeerConnection(conn);
}

/*-----------------------------------------------------------------------------
 * 描  述: 清除已经关闭的连接
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Policy::ClearInactiveConnections()
{
	//TODO: 这里加锁遍历肯定会影响性能的，后续优化
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
 * 描  述: 删除连接后的代理处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月22日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 根据规则选取连接
 * 返回值: [in] handler 选择规则
 * 修  改:
 *   时间 2013年10月26日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 通知其它的peer自己已经拥有了一个piece
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013.09.30
 *   作者 rosan
 *   描述 创建
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
 * 描  述:获取一个torrent上传总大小
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月1日
 *   作者 vincent_pan
 *   描述 创建
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
 * 描  述:获取一个torrent下载总大小
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月1日
 *   作者 vincent_pan
 *   描述 创建
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
 * 描  述:获取上传带宽
 * 参  数: 
 * 返回值: uint64 带宽值
 * 修  改:
 *   时间 2013年11月1日
 *   作者 vincent_pan
 *   描述 创建
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
 * 描  述:获取下载带宽
 * 参  数: 
 * 返回值: uint64 带宽值
 * 修  改:
 *   时间 2013年11月1日
 *   作者 vincent_pan
 *   描述 创建
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
 * 描  述:获取单个torrent下的inner_peer和outer_peer的数目 
 * 参  数: [out]   inner_peer_num
 		   [out]   outer_peer_num
 * 返回值: 
 * 修  改:
 *   时间 2013年11月1日
 *   作者 vincent_pan
 *   描述 创建
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

