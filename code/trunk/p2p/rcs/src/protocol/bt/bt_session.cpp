/*#############################################################################
 * �ļ���   : bt_session.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��9��13��
 * �ļ����� : BtSession��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <boost/bind.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "bt_session.hpp"
#include "bc_typedef.hpp"
#include "bc_util.hpp"
#include "rcs_util.hpp"
#include "rcs_config_parser.hpp"
#include "server_socket.hpp"
#include "bt_torrent.hpp"
#include "client_socket.hpp"
#include "bt_peer_connection.hpp"
#include "socket_server.hpp"
#include "socket_connection.hpp"
#include "ip_filter.hpp"
#include "communicator.hpp"
#include "message.pb.h"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtSession::BtSession()
{
	// ����session ����
	SetSessionType("bt");

	// ��������·��
	ParseSavePath();

    Communicator::GetInstance().RegisterConnectedHandler(
        boost::bind(&BtSession::OnUgsConnected, this));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtSession::~BtSession()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����socket������
 * ��  ��: [out] servers socket����������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtSession::CreateSocketServer(SocketServerVec& servers)
{
	// �������ļ���ȡ����IP:PORT
	std::string listen_addr_str;
	GET_RCS_CONFIG_STR("bt.common.listen-addr", listen_addr_str);

	std::vector<EndPoint> ep_vec = ParseEndPoints(listen_addr_str);
	if (ep_vec.empty())
	{
		LOG(ERROR) << "Invalid listen_addr_str";
		return false;
	}

    FOREACH(endpoint, ep_vec)
    {
	    // ���ݼ����˿ڴ���socket������
		SocketServerSP server(new TcpSocketServer(sock_ios(), endpoint, 
							  boost::bind(&Session::OnNewConnection, this, _1)));
		servers.push_back(server);
    }

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������
 * ��  ��: conn socket����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PeerConnectionSP BtSession::CreatePeerConnection(const SocketConnectionSP& conn)
{
	// �������ļ���ȡ���Ӵ������Ƽ��Ƿ�����peer
    AccessInfo access_info = IpFilter::GetInstance().Access(
        ip_address(ipv4_address(conn->connection_id().remote.ip)));

	if (access_info.flags == BLOCKED) return PeerConnectionSP();

	PeerType peer_type = (access_info.flags == INNER) ? PEER_INNER : PEER_OUTER;

    return PeerConnectionSP(new BtPeerConnection(*this, conn, peer_type, 
            access_info.download_speed_limit * 1024,
            access_info.upload_speed_limit * 1024));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������
 * ��  ��: remote peer��ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PeerConnectionSP BtSession::CreatePeerConnection(const EndPoint& remote, PeerType peer_type)
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
		//TODO: ������ʱд��
		download_speed_limit = 1024 * 1024;
		upload_speed_limit   = 1024 * 1024;
	}

	SocketConnectionSP sock_conn(new TcpSocketConnection(sock_ios(), remote));

	return PeerConnectionSP(new BtPeerConnection(*this, sock_conn, peer_type, 
            download_speed_limit, upload_speed_limit));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����BtTorrent
 * ��  ��: [in] hash torrent��InfoHash
 *         [in] save_path torrent����·��
 * ����ֵ: BtTorrentָ��
 * ��  ��:
 *   ʱ�� 2013��09��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
TorrentSP BtSession::CreateTorrent(const InfoHashSP& hash, const fs::path& save_path)
{
	return TorrentSP(new BtTorrent(*this, hash, save_path));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����BTЭ��Session�ı���·��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��10��31��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtSession::ParseSavePath()
{
	// �������ļ���ȡBT��Դ�ļ��ı���·��
	std::string save_path;
	GET_RCS_CONFIG_STR("bt.common.save-path", save_path);

	if (save_path.empty())
	{
		LOG(ERROR) << "Empty save path string";
		return;
	}

	// ��������·��
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

		// ���·���������򴴽�·��
		if (!fs::exists(full_path, ec))
		{
			if (!fs::create_directory(full_path, ec))
			{
				LOG(ERROR) << "Fail to create BT session save path | " << full_path;
				continue;
			}
		}

    	save_paths_.push_back(full_path);
	}

	LOG(INFO) << "Success to parse BT session save path";
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡָ��·��������Torrent��InfoHash
 * ��  ��: [in] save_path ·��
 * ����ֵ: InfoHash
 * ��  ��:
 *   ʱ�� 2013��10��31��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Session::InfoHashVec BtSession::GetTorrentInfoHash(const fs::path& save_path)
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
		if (hash_str.size() != 40) // ·����Ӧ�ö���40λinfohashĿ¼
		{
			LOG(WARNING) << "Invalid path string length | " << hash_str;
			continue;
		}
		hash_vec.push_back(InfoHashSP(new BtInfoHash(FromHex(hash_str))));
	}

	return hash_vec;
}

/*-----------------------------------------------------------------------------
 * ��  ? �ж�path�����·���Ƿ�Ϊsave_path�����ϼ���·��  ���� /disk
 * ��  ��: [in] path ·��
 * ����ֵ: bool 
 * ��  ��:
 *   ʱ�� 2013��11��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
SavePathVec BtSession::GetSavePath()
{
	return save_paths_;
}

std::string BtSession::GetSessionProtocol()
{
	return "BT";
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡpeer-id�ֽ���
 * ��  ��:
 * ����ֵ: peer-id�ֽ���
 * ��  ��:
 *   ʱ�� 2013.11.18
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
constexpr uint32 BtSession::GetPeerIdLength()
{
    return 20;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ��RCS�ͻ��˵�peer-id
 * ��  ��:
 * ����ֵ: ��RCS�ͻ��˵�peer-id
 * ��  ��:
 *   ʱ�� 2013.11.18
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
const char* BtSession::GetLocalPeerId()
{
    static const uint32 kPeerIdLength = GetPeerIdLength();

    static char peer_id[kPeerIdLength] = "BI/";  // ���ͻ��˵�peer-id
    static bool peer_id_generated = false;  // peer-id�Ƿ��Ѿ�����

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
 * ��  ��: �ϱ�������Դ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtSession::ReportResource()
{
	// �������ļ���ȡ����IP:PORT
	std::string listen_addr_str;
	GET_RCS_CONFIG_STR("bt.common.listen-addr", listen_addr_str);

    LOG(INFO) << "Report service address.";
    BtReportServiceAddressMsg report_address_msg;
	std::vector<EndPoint> ep_vec = ParseEndPoints(listen_addr_str);

    FOREACH(endpoint, ep_vec)
    {
        EndPointStruct* ep = report_address_msg.add_rcs();
        ep->set_ip(endpoint.ip);
        ep->set_port(endpoint.port);
        LOG(INFO) << endpoint;
    }

    Communicator::GetInstance().SendMsg(report_address_msg);


	// �������ļ���ȡBT��Դ�ļ��ı���·��
	std::string save_path;
	GET_RCS_CONFIG_STR("bt.common.save-path", save_path);

    LOG(INFO) << "Report file resource : ";
    BtReportResourceMsg msg;

	// ��������·��
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
 * ��  ��: ����keep-alive��Ϣ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtSession::SendKeepAlive()
{
	BtKeepAliveMsg keep_alive_msg;
    Communicator::GetInstance().SendMsg(keep_alive_msg);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��UGS�����Ϻ�Ļص�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtSession::OnUgsConnected()
{
    ReportResource();
}

}  // namespace BroadCache
