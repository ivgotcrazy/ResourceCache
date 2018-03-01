/*#############################################################################
 * 文件名   : session.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : Session声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEAER_SESSION
#define HEAER_SESSION

#include "depend.hpp"
#include "peer_connection.hpp"
#include "file_pool.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "endpoint.hpp"
#include "disk_io_thread.hpp"
#include "mem_buffer_pool.hpp"
#include "info_hash.hpp"
#include "torrent.hpp"
#include "timer.hpp"

namespace BroadCache
{

typedef std::vector<fs::path> SavePathVec;
typedef SavePathVec::iterator SavePathVecIter;

/******************************************************************************
 * 描述: 一个Session代表一种协议类型的会话
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
class Session : public boost::noncopyable,
	            public boost::enable_shared_from_this<Session>
{
public:
	typedef std::vector<SocketServerSP> SocketServerVec;
	typedef std::vector<EndPoint> ListenAddrVec;

	typedef std::vector<InfoHashSP> InfoHashVec;
	typedef InfoHashVec::iterator InfoHashVecIter;

public:
	Session();
	virtual ~Session();
	
	bool Start();
	void Stop();

	virtual PeerConnectionSP CreatePeerConnection(const SocketConnectionSP& conn) = 0; // 被动连接
	virtual PeerConnectionSP CreatePeerConnection(const EndPoint& remote, PeerType peer_type) = 0; // 主动连接

	TorrentSP NewTorrent(const InfoHashSP& hash);
	TorrentSP FindTorrent(const InfoHashSP& hash);
	void RemovePeerConnection(const ConnectionID& conn);
	void OnNewConnection(const SocketConnectionSP& conn);
	ListenAddrVec GetListenAddr();

	inline void SetSessionType(const std::string & s){ session_type_.assign(s); }

	//CPU资源调控函数，用于CPU使用率过高的情况 
	void StopNewConnnetcion();
	void ResumeNewConnnetcion(); 

	void CleanInactiveTorrentResource(const std::vector<std::string>& in_path_vec);

	bool FindHashInTorrents(std::string hash_str);

		//算出要删除的torrent目录路径
	void GetAdjustPath(std::string in_path, 
								std::vector<std::string>& path_vec);

	MemBufferPool& mem_buf_pool() { return mem_buf_pool_; }
	FilePool& file_pool() { return file_pool_; }
	DiskIoThread& io_thread() { return io_thread_; }
	io_service& sock_ios() { return sock_ios_; }
	io_service& work_ios() { return work_ios_; }
	io_service& timer_ios() { return timer_ios_; }

private:
	typedef boost::unordered_map<InfoHashSP, TorrentSP> TorrentMap;
	typedef TorrentMap::iterator TorrentMapIter;
	typedef std::pair<TorrentMapIter, bool> TorrentMapInsertRet;

	typedef boost::unordered_map<ConnectionID, PeerConnectionSP> PeerConnMap;
	typedef PeerConnMap::iterator PeerConnMapIter;

	typedef boost::unordered_map<InfoHashSP, uint64> SavePathMap;
	typedef SavePathMap::iterator SavePathMapIter;


private: // 内部函数
	void OnTick();
	void CreateThreads();
	void IoServiceThread(io_service& ios, std::string thread_name);
	void ClearInactiveTorrents();
	void CallTorrentsTickProc();
	bool InitTorrentPathMap();
	fs::path GetTorrentSavePath(const InfoHashSP& info_hash);
	bool CanAcceptConnection(const SocketConnectionSP& conn);
	virtual void SendKeepAlive() = 0;

private: // 协议模块实现接口
	virtual InfoHashVec GetTorrentInfoHash(const fs::path& save_path) = 0;
	virtual bool CreateSocketServer(SocketServerVec& servers) = 0;
	virtual TorrentSP CreateTorrent(const InfoHashSP& hash, const fs::path& save_path) = 0;
	virtual SavePathVec GetSavePath() = 0;
	virtual std::string GetSessionProtocol() = 0;

private:

	// 管理Session下所有Torrent
	TorrentMap torrents_;
	boost::mutex torrent_mutex_;
	
	// 管理所有打开的Torrent文件，不包括fast-resume和metadata
	FilePool file_pool_;
	//session type
	std::string session_type_;
	// socket相关
	std::vector<boost::thread> sock_threads_;
	boost::asio::io_service	sock_ios_;
	
	// session工作线程相关
	boost::thread work_thread_;	
	io_service work_ios_;

	// session定时器相关
	boost::thread timer_thread_;
	io_service timer_ios_;
	Timer timer_;
	
	MemBufferPool mem_buf_pool_;		// session内部使用内存池

	DiskIoThread io_thread_;			// IO操作管理对象，内部使用独立线程

	SocketServerVec socket_servers_;	// socket服务器，监听服务

	SavePathMap torrent_path_map_;		// Session下所有Torrent的保存路径

	bool stop_flag_;  // 用来控制Session线程的退出
	bool stop_new_connection_flag_; // 停止接收新的连接 

	// 被动连接管理
	PeerConnMap peer_conn_map_;
	boost::mutex peer_conn_mutex_;

	friend class GetBcrmInfo;
};

}

#endif
