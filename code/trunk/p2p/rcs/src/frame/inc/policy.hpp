/*#############################################################################
 * 文件名   : policy.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月14日
 * 文件描述 : Policy类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_POLICY
#define HEADER_POLICY

#include "depend.hpp"
#include "peer.hpp"
#include "endpoint.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "torrent.hpp"
#include "connection.hpp"
#include "peer_connection.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: peer连接策略
 * 作者：teck_zhou
 * 时间：2013/10/14
 *****************************************************************************/
class Policy : public boost::noncopyable,
			   public boost::enable_shared_from_this<Policy>
{
public:
	typedef boost::function<bool(const PeerConnectionSP& conn)> SelectHandler;

public:
	Policy(const TorrentSP& torrent);
	~Policy();

	void OnTick();

	void SetSeed();
	
	void HavePiece(uint32 piece);
	
	void AddPeer(const EndPoint& peer, PeerType peer_type, PeerRetriveType retrive_type);
	
	void IncommingPeerConnecton(const PeerConnectionSP& conn);

	std::vector<PeerConnectionSP> SelectConnection(SelectHandler handler);

	void set_stop_new_connection_flag(bool block) { blocked_for_monitor_ = block; }

	// 获取一个torrent下面的上传和下载带宽
	uint64 GetUploadBandwidth();
	uint64 GetDownloadBandwidth(); 

	// 获取一个torrent下上传和下载的大小
	uint64 GetUploadTotalSize();
	uint64 GetDownloadTotalSize();

	// 获取inner_peer_num 和outer_peer_num的数目
	void GetInOutPeerNum(uint32* iner_peer_num, uint32* outer_peer_num);
	
	uint64 GetPeerConnNum();
	uint64 GetPassivePeerConnNum();
	uint32 GetProxyPeerConnNum();

private:
	struct PeerEntry
	{
		PeerEntry() 
			: peer_type(PEER_UNKNOWN)
			, retrive_type(PEER_RETRIVE_UNKNOWN)
			, tried_times(0) {}

		PeerEntry(EndPoint addr, PeerType pt, PeerRetriveType rt)
			: address(addr)
			, peer_type(pt)
			, retrive_type(rt)
			, tried_times(0) {}

		EndPoint address;				// IP:PORT
		PeerType peer_type;				// 类型
		PeerRetriveType retrive_type;	// 以何种方式获取的
		ptime last_connect;				// 上次连接的时间
		uint8 tried_times;				// 尝试连接的次数
		PeerConnectionSP conn;			// 关联的PeerConnection
	};

	struct PeerEntryCmp
	{
		bool operator()(const PeerEntry& lhs, const PeerEntry& rhs)
		{
			return lhs.address < rhs.address;
		}
	};

	typedef std::set<PeerEntry, PeerEntryCmp> PeerSet;
	typedef PeerSet::iterator PeerSetIter;

	typedef boost::unordered_map<EndPoint, PeerEntry> PeerMap;
	typedef PeerMap::iterator PeerMapIter;

	typedef boost::unordered_map<ConnectionID, PeerConnectionSP> PeerConnMap;
	typedef PeerConnMap::iterator PeerConnMapIter;

private:
	bool GetOneCandidatePeer(PeerSet& peers, PeerEntry& peer);
	
	bool IfNeedMoreInnerPeers();
	bool IfNeedMoreOuterPeers();
	bool IsBlocked() const { return blocked_for_monitor_ || blocked_for_proxy_; }

	void ConnectOnePeer(const PeerEntry& peer);
	void OnActiveConnectionConnected(ConnectionError error, PeerEntry peer, const PeerConnectionWP& peer_conn);

	void PeerConnected(PeerEntry peer, const PeerConnectionSP& conn);
	void FailToConnectPeer(PeerEntry peer, const PeerConnectionSP& peer_conn);

	void AddPeerConnection(const PeerConnectionSP& conn);
	void ConnectionClosed(const PeerConnectionSP& conn);

	void UpdateFailedPeers();
	void ClearOuterPeers();
	void ClearOuterConnections();
	void ClearInactiveConnections();
	void CallConnectionTickProc();

	void TryRetrieveConnectablePeers();
	void TryConnectPeers();

	void TryRetrivePeers(const TorrentSP& torrent);
	void TryRetriveCachedRCSes(const TorrentSP& torrent);
	void TryRetriveInnerProxies(const TorrentSP& torrent);
	void TryRetriveOuterProxies(const TorrentSP& torrent);
	
	void TryConnectInnerPeers();
	void TryConnectOuterPeers();
	void TryConnectCachedRCSes();
	void TryConnectInnerProxies();
	void TryConnectOuterProxies();

	void ProxyProcForRemovedConn();

private:
	TorrentWP torrent_;

	PeerSet candidate_inner_peers_;		// 可供主动连接的内网peer
	PeerSet candidate_outer_peers_;		// 可供主动连接的外网peer
	PeerSet candidate_cached_rcses_;	// 可供主动连接的RCS
	PeerSet candidate_inner_proxies_;	// 可供主动连接的本地proxy
	PeerSet candidate_outer_proxies_;	// 可供主动连接的外部proxy

	PeerMap connected_peers_;	// 已经建立主动连接的peer
	PeerMap failed_peers_;		// 尝试连接失败的peer

	boost::mutex peer_mutex_;

	ptime last_retrive_peers_time_;

	// 连接管理，包括主动连接和被动连接
	PeerConnMap peer_conn_map_;
	boost::mutex peer_conn_mutex_;

	// 资源监控的连接控制
	bool blocked_for_monitor_;

	// proxy类型torrent的连接控制
	bool blocked_for_proxy_;

	bool retrived_cached_rcses_;
	bool retrived_inner_proxies_;
	bool retrived_outer_proxies_;

	ptime retrive_inner_proxy_time_;
	uint32 use_proxy_download_speed_;

	friend class GetBcrmInfo;
};

}

#endif
