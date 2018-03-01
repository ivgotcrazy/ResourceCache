/*#############################################################################
 * 文件名   : pps_session.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月25日
 * 文件描述 : PpsSession类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <boost/bind.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "pps_session.hpp"
#include "bc_typedef.hpp"
#include "bc_util.hpp"
#include "rcs_util.hpp"
#include "rcs_config_parser.hpp"
#include "server_socket.hpp"
#include "pps_torrent.hpp"
#include "client_socket.hpp"
#include "pps_peer_connection.hpp"
#include "socket_server.hpp"
#include "socket_connection.hpp"
#include "ip_filter.hpp"
#include "communicator.hpp"
#include "message.pb.h"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月14日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PpsSession::PpsSession()
{
	// 设置session 类型
	SetSessionType("pps");

	// 解析保存路径
	ParseSavePath();

    Communicator::GetInstance().RegisterConnectedHandler(
        boost::bind(&PpsSession::OnUgsConnected, this));
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月14日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PpsSession::~PpsSession()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建socket服务器
 * 参  数: [out] servers socket服务器容器
 * 返回值:
 * 修  改:
 *   时间 2013年10月14日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsSession::CreateSocketServer(SocketServerVec& servers)
{
	// 从配置文件读取监听IP:PORT
	std::string listen_addr_str;
	GET_RCS_CONFIG_STR("pps.common.listen-addr", listen_addr_str);

	std::vector<EndPoint> ep_vec = ParseEndPoints(listen_addr_str);
	if (ep_vec.empty())
	{
		LOG(ERROR) << "Invalid listen_addr_str";
		return false;
	}

    FOREACH(endpoint, ep_vec)
    {
	    // 根据监听端口创建socket服务器
		SocketServerSP server(new UdpSocketServer(sock_ios(), endpoint, 
							  boost::bind(&Session::OnNewConnection, this, _1)));
		servers.push_back(server);
    }

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建被动连接
 * 参  数: conn socket连接
 * 返回值:
 * 修  改:
 *   时间 2013年10月13日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PeerConnectionSP PpsSession::CreatePeerConnection(const SocketConnectionSP& conn)
{
	// 从配置文件获取连接带宽限制即是否内网peer
    AccessInfo access_info = IpFilter::GetInstance().Access(
        ip_address(ipv4_address(conn->connection_id().remote.ip)));

	if (access_info.flags == BLOCKED) return PeerConnectionSP();

	PeerType peer_type = (access_info.flags == INNER) ? PEER_INNER : PEER_OUTER;

    return PeerConnectionSP(new PpsPeerConnection(*this, conn, peer_type, 
            access_info.download_speed_limit * 1024,
            access_info.upload_speed_limit * 1024));
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建主动连接
 * 参  数: remote peer地址
 * 返回值:
 * 修  改:
 *   时间 2013年10月13日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PeerConnectionSP PpsSession::CreatePeerConnection(const EndPoint& remote, PeerType peer_type)
{
	uint64 download_speed_limit = 0, upload_speed_limit = 0;

	IpFilter& filter = IpFilter::GetInstance();
	
	if (peer_type == PEER_INNER || peer_type == PEER_OUTER)
	{
		AccessInfo access_info = filter.Access(ip_address(ipv4_address(remote.ip)));

		download_speed_limit = access_info.download_speed_limit * 1024;
		upload_speed_limit   = access_info.upload_speed_limit * 1024;
	}
	else // PEER_RCS, PEER_INNER_PROXY, PEER_OUTER_PROXY
	{
		//TODO: 这里暂时写死
		download_speed_limit = 1024 * 1024;
		upload_speed_limit   = 1024 * 1024;
	}

	SocketConnectionSP sock_conn(new UdpSocketConnection(sock_ios(), remote));

	return PeerConnectionSP(new PpsPeerConnection(*this, sock_conn, peer_type, 
            download_speed_limit, upload_speed_limit));
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建BtTorrent
 * 参  数: [in] hash torrent的InfoHash
 *         [in] save_path torrent保存路径
 * 返回值: BtTorrent指针
 * 修  改:
 *   时间 2013年09月10日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
TorrentSP PpsSession::CreateTorrent(const InfoHashSP& hash, const fs::path& save_path)
{
	return TorrentSP(new PpsTorrent(*this, hash, save_path));
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析Pps协议Session的保存路径
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsSession::ParseSavePath()
{
	// 从配置文件读取Pps资源文件的保存路径
	std::string save_path;
	GET_RCS_CONFIG_STR("pps.common.save-path", save_path);

	if (save_path.empty())
	{
		LOG(ERROR) << "Empty save path string";
		return;
	}

	// 解析配置路径
	std::vector<std::string> str_vec = SplitStr(save_path, ' ');
	for (std::string& str : str_vec)
	{
		error_code ec;
		fs::path full_path = fs::system_complete(fs::path(str), ec);
		if (ec)
		{
			LOG(ERROR) << "Fail to get complete path | " << str;
			continue;
		}

		// 如果路径不存在则创建路径
		if (!fs::exists(full_path, ec))
		{
			if (!fs::create_directory(full_path, ec))
			{
				LOG(ERROR) << "Fail to create Pps session save path | " << full_path;
				continue;
			}
		}

    	save_paths_.push_back(full_path);
	}

	LOG(INFO) << "Success to parse PPS session save path";
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取指定路径下所有Torrent的InfoHash
 * 参  数: [in] save_path 路径
 * 返回值: InfoHash
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
Session::InfoHashVec PpsSession::GetTorrentInfoHash(const fs::path& save_path)
{
	if (!fs::is_directory(save_path))
	{
		LOG(ERROR) << "Path is not a directory | " << save_path;
		return InfoHashVec();
	}

	InfoHashVec hash_vec;

	fs::directory_iterator end;
	for (fs::directory_iterator pos(save_path); pos != end; ++pos)
	{
		fs::path path(*pos);
		if (!fs::is_directory(path))
		{
			LOG(WARNING) << "Unexpected path | " << path;
			continue;
		}

		std::string hash_str = path.filename().string();
		if (hash_str.size() != 40) // 路径下应该都是40位infohash目录
		{
			LOG(WARNING) << "Invalid path string length | " << hash_str;
			continue;
		}
		hash_vec.push_back(InfoHashSP(new PpsInfoHash(FromHex(hash_str))));

	}

	return hash_vec;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取保存路径
 * 参  数: 
 * 返回值: 保存路径
 * 修  改:
 *   时间 2013年11月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
SavePathVec PpsSession::GetSavePath()
{
	return save_paths_;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取协议名称
 * 参  数: 
 * 返回值: 协议名称
 * 修  改:
 *   时间 2013年12月23日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
std::string PpsSession::GetSessionProtocol()
{
	return "PPS";
}

/*------------------------------------------------------------------------------
 * 描  述: 获取peer-id字节数
 * 参  数:
 * 返回值: peer-id字节数
 * 修  改:
 *   时间 2013.12.23
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
constexpr uint32 PpsSession::GetPeerIdLength()
{
    return 16;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取本RCS客户端的peer-id
 * 参  数:
 * 返回值: 本RCS客户端的peer-id
 * 修  改:
 *   时间 2013.12.23
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
const char* PpsSession::GetLocalPeerId()
{
    static const uint32 kPeerIdLength = GetPeerIdLength();

    static char peer_id[kPeerIdLength] = "BI/";  // 本客户端的peer-id
    static bool peer_id_generated = false;  // peer-id是否已经生成

    if (!peer_id_generated)
    {
        boost::random::mt19937 gen;
        boost::random::uniform_int_distribution<> dist(0, 9);
        
        for (uint32 i=3; i<kPeerIdLength; ++i)
        {
            peer_id[i] = static_cast<char>(dist(gen) + '0');
        } 

        peer_id_generated = true;
    }

    return peer_id;
}

/*------------------------------------------------------------------------------
 * 描  述: 上报本地资源
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.23
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsSession::ReportResource()
{
	// 从配置文件读取监听IP:PORT
	std::string listen_addr_str;
	GET_RCS_CONFIG_STR("pps.common.listen-addr", listen_addr_str);

    LOG(INFO) << "Report service address.";
    PpsReportServiceAddressMsg report_address_msg;
	std::vector<EndPoint> ep_vec = ParseEndPoints(listen_addr_str);

    FOREACH(endpoint, ep_vec)
    {
        EndPointStruct* ep = report_address_msg.add_rcs();
        ep->set_ip(endpoint.ip);
        ep->set_port(endpoint.port);
        LOG(INFO) << endpoint;
    }

    Communicator::GetInstance().SendMsg(report_address_msg);

	// 从配置文件读取Pps资源文件的保存路径
	std::string save_path;
	GET_RCS_CONFIG_STR("pps.common.save-path", save_path);

    LOG(INFO) << "Report file resource :";
    PpsReportResourceMsg msg;

	// 解析配置路径
	std::vector<std::string> str_vec = SplitStr(save_path, ' ');
	for (std::string& str : str_vec)
	{
		error_code ec;
		fs::path full_path = fs::system_complete(fs::path(str), ec);
		if (ec || !fs::exists(full_path) || !fs::is_directory(full_path))
		{
			continue;
		}

        fs::directory_iterator i(full_path);
        fs::directory_iterator end;
        for (; i != end; ++i)
        {
            static const boost::regex kRegexInfoHash("[A-Za-z0-9]{40}");

            fs::path entry = i->path();
            std::string filename = entry.filename().string();
            if (fs::is_directory(entry) 
                    && boost::regex_match(filename, kRegexInfoHash))
            {
                msg.add_info_hash(FromHex(filename));
                LOG(INFO) << filename;
            }
        }
    }

    Communicator::GetInstance().SendMsg(msg);
}

/*------------------------------------------------------------------------------
 * 描  述: 发送keep-alive消息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsSession::SendKeepAlive()
{
	PpsKeepAliveMsg keep_alive_msg;
    Communicator::GetInstance().SendMsg(keep_alive_msg);
}


/*------------------------------------------------------------------------------
 * 描  述: 当UGS连接上后的回调函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnUgsConnected()
{
    ReportResource();
}

}  // namespace BroadCache
