/*#############################################################################
 * �ļ���   : session.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : Session����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ����: һ��Session����һ��Э�����͵ĻỰ
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
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

	virtual PeerConnectionSP CreatePeerConnection(const SocketConnectionSP& conn) = 0; // ��������
	virtual PeerConnectionSP CreatePeerConnection(const EndPoint& remote, PeerType peer_type) = 0; // ��������

	TorrentSP NewTorrent(const InfoHashSP& hash);
	TorrentSP FindTorrent(const InfoHashSP& hash);
	void RemovePeerConnection(const ConnectionID& conn);
	void OnNewConnection(const SocketConnectionSP& conn);
	ListenAddrVec GetListenAddr();

	inline void SetSessionType(const std::string & s){ session_type_.assign(s); }

	//CPU��Դ���غ���������CPUʹ���ʹ��ߵ���� 
	void StopNewConnnetcion();
	void ResumeNewConnnetcion(); 

	void CleanInactiveTorrentResource(const std::vector<std::string>& in_path_vec);

	bool FindHashInTorrents(std::string hash_str);

		//���Ҫɾ����torrentĿ¼·��
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


private: // �ڲ�����
	void OnTick();
	void CreateThreads();
	void IoServiceThread(io_service& ios, std::string thread_name);
	void ClearInactiveTorrents();
	void CallTorrentsTickProc();
	bool InitTorrentPathMap();
	fs::path GetTorrentSavePath(const InfoHashSP& info_hash);
	bool CanAcceptConnection(const SocketConnectionSP& conn);
	virtual void SendKeepAlive() = 0;

private: // Э��ģ��ʵ�ֽӿ�
	virtual InfoHashVec GetTorrentInfoHash(const fs::path& save_path) = 0;
	virtual bool CreateSocketServer(SocketServerVec& servers) = 0;
	virtual TorrentSP CreateTorrent(const InfoHashSP& hash, const fs::path& save_path) = 0;
	virtual SavePathVec GetSavePath() = 0;
	virtual std::string GetSessionProtocol() = 0;

private:

	// ����Session������Torrent
	TorrentMap torrents_;
	boost::mutex torrent_mutex_;
	
	// �������д򿪵�Torrent�ļ���������fast-resume��metadata
	FilePool file_pool_;
	//session type
	std::string session_type_;
	// socket���
	std::vector<boost::thread> sock_threads_;
	boost::asio::io_service	sock_ios_;
	
	// session�����߳����
	boost::thread work_thread_;	
	io_service work_ios_;

	// session��ʱ�����
	boost::thread timer_thread_;
	io_service timer_ios_;
	Timer timer_;
	
	MemBufferPool mem_buf_pool_;		// session�ڲ�ʹ���ڴ��

	DiskIoThread io_thread_;			// IO������������ڲ�ʹ�ö����߳�

	SocketServerVec socket_servers_;	// socket����������������

	SavePathMap torrent_path_map_;		// Session������Torrent�ı���·��

	bool stop_flag_;  // ��������Session�̵߳��˳�
	bool stop_new_connection_flag_; // ֹͣ�����µ����� 

	// �������ӹ���
	PeerConnMap peer_conn_map_;
	boost::mutex peer_conn_mutex_;

	friend class GetBcrmInfo;
};

}

#endif
