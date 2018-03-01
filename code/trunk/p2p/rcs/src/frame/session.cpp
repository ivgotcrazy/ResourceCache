/*#############################################################################
 * 文件名   : session.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : Session实现文件
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Session::~Session()
{
	LOG(INFO) << ">>> Destructing session";

	Stop();
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器消息处理函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Session::OnTick()
{
	SendKeepAlive();
	ClearInactiveTorrents();
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建线程
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月10日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 启动Session
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 停止Session
 * 参  数:
 * 返回值: 
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 创建torrent，并发起获取torrent元数据
 * 参  数: [in] hash	InfoHash
 * 返回值: 
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
TorrentSP Session::NewTorrent(const InfoHashSP& hash)
{
	LOG(INFO) << "Add one torrent to session | " << hash->to_string();

	// 规格限制

	fs::path torrent_path = GetTorrentSavePath(hash);

	TorrentSP torrent = CreateTorrent(hash, torrent_path);
	if (!torrent->Init())
	{
		LOG(ERROR) << "Fail to init torrent | " << hash->to_string();
		return TorrentSP();
	}

	// 将torrent保存到管理容器
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
 * 描  述: io_service运行线程
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Session::IoServiceThread(io_service& ios, std::string thread_name)
{
	LOG(INFO) << "Start " << thread_name << " thread...";

	boost::asio::io_service::work worker(ios);

	ios.run();

	LOG(INFO) << "Stop " << thread_name << " thread...";
}

/*-----------------------------------------------------------------------------
 * 描  述: 是否能够接受新连接
 * 参  数: [in] conn 连接
 * 返回值: 是/否
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Session::CanAcceptConnection(const SocketConnectionSP& conn)
{
	if (stop_new_connection_flag_) return false;

	// 判断是否是禁用IP
	ip_address addr = to_address(conn->connection_id().remote.ip);
	AccessInfo access_info = IpFilter::GetInstance().Access(addr);
	if (access_info.flags == BLOCKED) return false;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 新建连接处理
 * 参  数: conn 连接
 * 返回值:
 * 修  改:
 *   时间 2013年10月13日
 *   作者 teck_zhou
 *   描述 创建
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

	// 被动连接在没有处理报文之前是无法知道此连接属于哪个torrent的，因此先保存起来
	peer_conn_map_.insert(PeerConnMap::value_type(conn->connection_id(), peer_conn));

	conn->Connect(); // 对于被动连接来说，连接已经建立，开始接受报文

	peer_conn->Init();

	peer_conn->Start();
}

/*-----------------------------------------------------------------------------
 * 描  述: 停止新连接的接收和发送
 * 参  数: block 是否阻塞
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Session::StopNewConnnetcion() 
{ 
	stop_new_connection_flag_ = true;
	for (auto torrent : torrents_)
	{
		//设置 session下的每一个torrent的block
		torrent.second->StopLaunchNewConnection();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 恢复新连接的接蘸头⑺? * 参  数: block 是否阻塞
 * 返回值:
 * 修  改:
 *   时间 2013年10月28日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Session::ResumeNewConnnetcion() 
{ 
	stop_new_connection_flag_ = false;
	for (auto torrent : torrents_)
	{
		//设置 session下的每一个torrent的block
		torrent.second->ResumeLaunchNewConnection();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: Torrent会定时清理无效连接，清理之前要通知Session
 * 参  数: [in] conn	连接
 * 返回值: 
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 一个torrent如果长时间没有peer来连接，则需要将其卸载
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 通过InfoHash查找Torrent
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 获取监听地址
 * 参  数: 
 * 返回值: 所有监听地址
 * 修  改:
 *   时间 2013年10月30日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 初始化torrent存储路径的hash映射
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年10月31日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Session::InitTorrentPathMap()
{
	SavePathVec save_paths = GetSavePath();

	// 调用协议模块接口获取协议资源数据保存路径
	if (save_paths.empty())
	{
		LOG(ERROR) << "Fail to get session save path";
		return false;
	}

	// 构建当前已存在torrent的path的hash映射
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
 * 描  述: 获取torrent的存储路径
 * 参  数: [in] info_hash hash值
 * 返回值: torrent存储路径
 * 修  改:
 *   时间 2013年10月31日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
fs::path Session::GetTorrentSavePath(const InfoHashSP& info_hash)
{
	SavePathVec save_paths = GetSavePath();
	BC_ASSERT(!save_paths.empty());

	// 先从map中查找，找到则直接返回
	SavePathMapIter iter = torrent_path_map_.find(info_hash);
	if (iter != torrent_path_map_.end())
	{
		LOG(INFO) << "Get torrent save path from hash map | path_index: " << iter->second;
		BC_ASSERT(iter->second < save_paths.size());
		return save_paths[iter->second];
	}

	// 没找到则使用算法将torrent哈希到某一个路径
	// 当前的hash算法非常简单，直接使用hash值的最后一个字节取模
	std::string hash_str = info_hash->raw_data();
	uint64 path_index = hash_str[0] % save_paths.size();

	// 将映射结果保存
	torrent_path_map_.insert(SavePathMap::value_type(info_hash, path_index));

	LOG(INFO) << "Get torrent save path by hashing | path_index: " << path_index;

	return save_paths[path_index];
}

/*-----------------------------------------------------------------------------
 * 描  述: 判断path传入的路径是否为save_path的最上级父路径  例如 /disk
 * 参  数: [in] path 路径
 * 返回值: bool 
 * 修  改:
 *   时间 2013年11月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Session::GetAdjustPath(const std::string in_path, 
													std::vector<std::string>& out_path_vec)
{
	std::string  path_root;
		//算出要删除的torrent目录路径
	SavePathVec save_path = GetSavePath();
	for (auto path : save_path)
	{	
		//判断path路径中是否包含传入的路径,例如 /disk1
		if (path.string().find(in_path) != path.string().npos)
		{
			LOG(INFO) << "Save_path have the str";
			out_path_vec.push_back(path.string());
		}
	}
	
	LOG(INFO) << "Calc save path | size:" << out_path_vec.size();
}

/*-----------------------------------------------------------------------------
 * 描  述: 查找hash_str代表的infohash是否在TorrentMap中
 * 参  数: [in] hash_str  infohash字符串
 * 返回值: bool 
 * 修  改:
 *   时间 2013年11月2日
 *   作者 tom_liu
 *   描述 创建
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
