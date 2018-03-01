/*#############################################################################
 * �ļ���   : policy.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��14��
 * �ļ����� : Policy������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ����: peer���Ӳ���
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/14
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

	// ��ȡһ��torrent������ϴ������ش���
	uint64 GetUploadBandwidth();
	uint64 GetDownloadBandwidth(); 

	// ��ȡһ��torrent���ϴ������صĴ�С
	uint64 GetUploadTotalSize();
	uint64 GetDownloadTotalSize();

	// ��ȡinner_peer_num ��outer_peer_num����Ŀ
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
		PeerType peer_type;				// ����
		PeerRetriveType retrive_type;	// �Ժ��ַ�ʽ��ȡ��
		ptime last_connect;				// �ϴ����ӵ�ʱ��
		uint8 tried_times;				// �������ӵĴ���
		PeerConnectionSP conn;			// ������PeerConnection
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

	PeerSet candidate_inner_peers_;		// �ɹ��������ӵ�����peer
	PeerSet candidate_outer_peers_;		// �ɹ��������ӵ�����peer
	PeerSet candidate_cached_rcses_;	// �ɹ��������ӵ�RCS
	PeerSet candidate_inner_proxies_;	// �ɹ��������ӵı���proxy
	PeerSet candidate_outer_proxies_;	// �ɹ��������ӵ��ⲿproxy

	PeerMap connected_peers_;	// �Ѿ������������ӵ�peer
	PeerMap failed_peers_;		// ��������ʧ�ܵ�peer

	boost::mutex peer_mutex_;

	ptime last_retrive_peers_time_;

	// ���ӹ��������������Ӻͱ�������
	PeerConnMap peer_conn_map_;
	boost::mutex peer_conn_mutex_;

	// ��Դ��ص����ӿ���
	bool blocked_for_monitor_;

	// proxy����torrent�����ӿ���
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
