/*#############################################################################
 * �ļ���   : session.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : Sessionʵ���ļ�
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "session.hpp"
#include "info_hash.hpp"
#include "torrent.hpp"
#include "bc_assert.hpp"
#include "connection.hpp"
#include "socket_connection.hpp"
#include "socket_server.hpp"
#include "disk_io_thread.hpp"
#include "policy.hpp"
#include "ip_filter.hpp"
#include "rcs_config_parser.hpp"
#include "rcs_config.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Session::Session() 
	: sock_ios_()
	, work_ios_()
	, timer_ios_()
	, timer_(timer_ios_, boost::bind(&Session::OnTick, this), 1)
	, mem_buf_pool_(MB_MSG_COMMON_SIZE, MB_MSG_DATA_SIZE, MB_DATA_BLOCK_SIZE)
	, io_thread_(mem_buf_pool_)
	, stop_new_connection_flag_(false)
	 
{
	session_type_ = "";
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Session::~Session()
{
	LOG(INFO) << ">>> Destructing session";

	Stop();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����Ϣ������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::OnTick()
{
	SendKeepAlive();
	ClearInactiveTorrents();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����߳�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::CreateThreads()
{
	timer_thread_ = boost::thread(boost::bind(
		&Session::IoServiceThread, this, boost::ref(timer_ios_), "timer"));

	work_thread_ = boost::thread(boost::bind(
		&Session::IoServiceThread, this, boost::ref(work_ios_), "work"));

	uint32 thread_num;
	GET_RCS_CONFIG_INT("global.session.socket-thread-number", thread_num);

	for (uint64 i = 0; i < thread_num; i++)
	{
		sock_threads_.push_back(boost::thread(boost::bind(
			&Session::IoServiceThread, this, boost::ref(sock_ios_), "socket")));
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Session
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Session::Start()
{
	LOG(INFO) << "Start " << GetSessionProtocol() << " session...";

	if (!InitTorrentPathMap())
	{
		LOG(ERROR) << "Fail to initialize torrent path map";
		return false;
	}

	if (!CreateSocketServer(socket_servers_))
	{
		LOG(ERROR) << "Fail to create socket server";
		return false;
	}

	for (SocketServerSP& server : socket_servers_)
	{
		if(!server->Start())
		{
			LOG(ERROR) << "Fail to start socket server";
			return false;
		}
	}

	CreateThreads();

	io_thread_.Start();

	timer_.Start();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣSession
 * ��  ��:
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::Stop()
{
	work_ios_.stop();
	sock_ios_.stop();
	timer_ios_.stop();

	for (boost::thread& sock_thread : sock_threads_)
	{
		sock_thread.join();
	}

	work_thread_.join();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����torrent���������ȡtorrentԪ����
 * ��  ��: [in] hash	InfoHash
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
TorrentSP Session::NewTorrent(const InfoHashSP& hash)
{
	LOG(INFO) << "Add one torrent to session | " << hash->to_string();

	// �������

	fs::path torrent_path = GetTorrentSavePath(hash);

	TorrentSP torrent = CreateTorrent(hash, torrent_path);
	if (!torrent->Init())
	{
		LOG(ERROR) << "Fail to init torrent | " << hash->to_string();
		return TorrentSP();
	}

	// ��torrent���浽��������
	{
		boost::mutex::scoped_lock lock(torrent_mutex_);
		TorrentMapInsertRet result = torrents_.insert(TorrentMap::value_type(hash, torrent));
		if (!(result.second))
		{
			LOG(ERROR) << "Fail to insert torrent to session | " << hash->to_string();
			return TorrentSP();
		}

		LOG(INFO) << "Success to insert one torrent to session | " << hash->to_string();
	}

	return torrent;
}

/*-----------------------------------------------------------------------------
 * ��  ��: io_service�����߳�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::IoServiceThread(io_service& ios, std::string thread_name)
{
	LOG(INFO) << "Start " << thread_name << " thread...";

	boost::asio::io_service::work worker(ios);

	ios.run();

	LOG(INFO) << "Stop " << thread_name << " thread...";
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƿ��ܹ�����������
 * ��  ��: [in] conn ����
 * ����ֵ: ��/��
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Session::CanAcceptConnection(const SocketConnectionSP& conn)
{
	if (stop_new_connection_flag_) return false;

	// �ж��Ƿ��ǽ���IP
	ip_address addr = to_address(conn->connection_id().remote.ip);
	AccessInfo access_info = IpFilter::GetInstance().Access(addr);
	if (access_info.flags == BLOCKED) return false;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �½����Ӵ���
 * ��  ��: conn ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::OnNewConnection(const SocketConnectionSP& conn)
{
	if (!CanAcceptConnection(conn)) return;

	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	PeerConnMapIter iter = peer_conn_map_.find(conn->connection_id());
	if (iter != peer_conn_map_.end())
	{
		LOG(ERROR) << "Peer connection exists | " << *(conn.get());
		return;
	}

	PeerConnectionSP peer_conn = CreatePeerConnection(conn);
	if (!peer_conn)
	{
		LOG(ERROR) << "Fail to create peer connection | " << *(conn.get());
		return;
	}

	// ����������û�д�����֮ǰ���޷�֪�������������ĸ�torrent�ģ�����ȱ�������
	peer_conn_map_.insert(PeerConnMap::value_type(conn->connection_id(), peer_conn));

	conn->Connect(); // ���ڱ���������˵�������Ѿ���������ʼ���ܱ���

	peer_conn->Init();

	peer_conn->Start();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣ�����ӵĽ��պͷ���
 * ��  ��: block �Ƿ�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::StopNewConnnetcion() 
{ 
	stop_new_connection_flag_ = true;
	for (auto torrent : torrents_)
	{
		//���� session�µ�ÿһ��torrent��block
		torrent.second->StopLaunchNewConnection();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ָ������ӵĽ�պͷ��? * ��  ��: block �Ƿ�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::ResumeNewConnnetcion() 
{ 
	stop_new_connection_flag_ = false;
	for (auto torrent : torrents_)
	{
		//���� session�µ�ÿһ��torrent��block
		torrent.second->ResumeLaunchNewConnection();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: Torrent�ᶨʱ������Ч���ӣ�����֮ǰҪ֪ͨSession
 * ��  ��: [in] conn	����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::RemovePeerConnection(const ConnectionID& conn)
{
	boost::mutex::scoped_lock lock(peer_conn_mutex_);

	uint64 erase_num = peer_conn_map_.erase(conn);
	if (erase_num == 0)
	{
		LOG(ERROR) << "Peer connection to be removed not exists | " << conn;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: һ��torrent�����ʱ��û��peer�����ӣ�����Ҫ����ж��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::ClearInactiveTorrents()
{
	boost::mutex::scoped_lock lock(torrent_mutex_);

	TorrentMapIter iter = torrents_.begin();
	for ( ; iter != torrents_.end(); )
	{
		if (iter->second->IsInactive())
		{
			LOG(INFO) << "Removed one torrent from session";

			iter = torrents_.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ͨ��InfoHash����Torrent
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
TorrentSP Session::FindTorrent(const InfoHashSP& hash)
{
	boost::mutex::scoped_lock lock(torrent_mutex_);

	TorrentMapIter iter = torrents_.find(hash);
	if (iter == torrents_.end())
	{
		LOG(INFO) << "Fail to find torrent | " << hash->to_string();
		return TorrentSP();
	}

	return iter->second;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ������ַ
 * ��  ��: 
 * ����ֵ: ���м�����ַ
 * ��  ��:
 *   ʱ�� 2013��10��30��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Session::ListenAddrVec Session::GetListenAddr()
{
	ListenAddrVec addr_vec;
	for (SocketServerSP& server : socket_servers_)
	{
		addr_vec.push_back(server->local_address());
	}

	return addr_vec;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��torrent�洢·����hashӳ��
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��10��31��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Session::InitTorrentPathMap()
{
	SavePathVec save_paths = GetSavePath();

	// ����Э��ģ��ӿڻ�ȡЭ����Դ���ݱ���·��
	if (save_paths.empty())
	{
		LOG(ERROR) << "Fail to get session save path";
		return false;
	}

	// ������ǰ�Ѵ���torrent��path��hashӳ��
	uint64 path_index = 0;
	InfoHashVec hash_vec;
	for (fs::path& path : save_paths)
	{
		hash_vec.clear();
		hash_vec = GetTorrentInfoHash(path);
		if (hash_vec.empty())
		{
			LOG(WARNING) << "Found no torrent in path | " << path;
			path_index++;
			continue;
		}

		for (InfoHashSP& hash : hash_vec)
		{
			torrent_path_map_.insert(SavePathMap::value_type(hash, path_index));
		}

		path_index++;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡtorrent�Ĵ洢·��
 * ��  ��: [in] info_hash hashֵ
 * ����ֵ: torrent�洢·��
 * ��  ��:
 *   ʱ�� 2013��10��31��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
fs::path Session::GetTorrentSavePath(const InfoHashSP& info_hash)
{
	SavePathVec save_paths = GetSavePath();
	BC_ASSERT(!save_paths.empty());

	// �ȴ�map�в��ң��ҵ���ֱ�ӷ���
	SavePathMapIter iter = torrent_path_map_.find(info_hash);
	if (iter != torrent_path_map_.end())
	{
		LOG(INFO) << "Get torrent save path from hash map | path_index: " << iter->second;
		BC_ASSERT(iter->second < save_paths.size());
		return save_paths[iter->second];
	}

	// û�ҵ���ʹ���㷨��torrent��ϣ��ĳһ��·��
	// ��ǰ��hash�㷨�ǳ��򵥣�ֱ��ʹ��hashֵ�����һ���ֽ�ȡģ
	std::string hash_str = info_hash->raw_data();
	uint64 path_index = hash_str[0] % save_paths.size();

	// ��ӳ��������
	torrent_path_map_.insert(SavePathMap::value_type(info_hash, path_index));

	LOG(INFO) << "Get torrent save path by hashing | path_index: " << path_index;

	return save_paths[path_index];
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ж�path�����·���Ƿ�Ϊsave_path�����ϼ���·��  ���� /disk
 * ��  ��: [in] path ·��
 * ����ֵ: bool 
 * ��  ��:
 *   ʱ�� 2013��11��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Session::GetAdjustPath(const std::string in_path, 
													std::vector<std::string>& out_path_vec)
{
	std::string  path_root;
		//���Ҫɾ����torrentĿ¼·��
	SavePathVec save_path = GetSavePath();
	for (auto path : save_path)
	{	
		//�ж�path·�����Ƿ���������·��,���� /disk1
		if (path.string().find(in_path) != path.string().npos)
		{
			LOG(INFO) << "Save_path have the str";
			out_path_vec.push_back(path.string());
		}
	}
	
	LOG(INFO) << "Calc save path | size:" << out_path_vec.size();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����hash_str�����infohash�Ƿ���TorrentMap��
 * ��  ��: [in] hash_str  infohash�ַ���
 * ����ֵ: bool 
 * ��  ��:
 *   ʱ�� 2013��11��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Session::FindHashInTorrents(std::string hash_str)
{
	boost::mutex::scoped_lock lock(torrent_mutex_);

	TorrentMapIter iter = torrents_.begin();
	for ( ; iter != torrents_.end(); )
	{
		if (iter->first->to_string() == hash_str)
		{
			return true;
		}
	}
	return false;
}

}  // namespace BroadCache
