/*#############################################################################
 * �ļ���   : pps_session.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��25��
 * �ļ����� : PpsSession��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsSession::PpsSession()
{
	// ����session ����
	SetSessionType("pps");

	// ��������·��
	ParseSavePath();

    Communicator::GetInstance().RegisterConnectedHandler(
        boost::bind(&PpsSession::OnUgsConnected, this));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsSession::~PpsSession()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����socket������
 * ��  ��: [out] servers socket����������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsSession::CreateSocketServer(SocketServerVec& servers)
{
	// �������ļ���ȡ����IP:PORT
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
	    // ���ݼ����˿ڴ���socket������
		SocketServerSP server(new UdpSocketServer(sock_ios(), endpoint, 
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
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PeerConnectionSP PpsSession::CreatePeerConnection(const SocketConnectionSP& conn)
{
	// �������ļ���ȡ���Ӵ������Ƽ��Ƿ�����peer
    AccessInfo access_info = IpFilter::GetInstance().Access(
        ip_address(ipv4_address(conn->connection_id().remote.ip)));

	if (access_info.flags == BLOCKED) return PeerConnectionSP();

	PeerType peer_type = (access_info.flags == INNER) ? PEER_INNER : PEER_OUTER;

    return PeerConnectionSP(new PpsPeerConnection(*this, conn, peer_type, 
            access_info.download_speed_limit * 1024,
            access_info.upload_speed_limit * 1024));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������
 * ��  ��: remote peer��ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��13��
 *   ���� tom_liu
 *   ���� ����
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
		//TODO: ������ʱд��
		download_speed_limit = 1024 * 1024;
		upload_speed_limit   = 1024 * 1024;
	}

	SocketConnectionSP sock_conn(new UdpSocketConnection(sock_ios(), remote));

	return PeerConnectionSP(new PpsPeerConnection(*this, sock_conn, peer_type, 
            download_speed_limit, upload_speed_limit));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����BtTorrent
 * ��  ��: [in] hash torrent��InfoHash
 *         [in] save_path torrent����·��
 * ����ֵ: BtTorrentָ��
 * ��  ��:
 *   ʱ�� 2013��09��10��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
TorrentSP PpsSession::CreateTorrent(const InfoHashSP& hash, const fs::path& save_path)
{
	return TorrentSP(new PpsTorrent(*this, hash, save_path));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����PpsЭ��Session�ı���·��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsSession::ParseSavePath()
{
	// �������ļ���ȡPps��Դ�ļ��ı���·��
	std::string save_path;
	GET_RCS_CONFIG_STR("pps.common.save-path", save_path);

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
				LOG(ERROR) << "Fail to create Pps session save path | " << full_path;
				continue;
			}
		}

    	save_paths_.push_back(full_path);
	}

	LOG(INFO) << "Success to parse PPS session save path";
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡָ��·��������Torrent��InfoHash
 * ��  ��: [in] save_path ·��
 * ����ֵ: InfoHash
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
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
		if (hash_str.size() != 40) // ·����Ӧ�ö���40λinfohashĿ¼
		{
			LOG(WARNING) << "Invalid path string length | " << hash_str;
			continue;
		}
		hash_vec.push_back(InfoHashSP(new PpsInfoHash(FromHex(hash_str))));

	}

	return hash_vec;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����·��
 * ��  ��: 
 * ����ֵ: ����·��
 * ��  ��:
 *   ʱ�� 2013��11��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
SavePathVec PpsSession::GetSavePath()
{
	return save_paths_;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡЭ������
 * ��  ��: 
 * ����ֵ: Э������
 * ��  ��:
 *   ʱ�� 2013��12��23��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
std::string PpsSession::GetSessionProtocol()
{
	return "PPS";
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡpeer-id�ֽ���
 * ��  ��:
 * ����ֵ: peer-id�ֽ���
 * ��  ��:
 *   ʱ�� 2013.12.23
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
constexpr uint32 PpsSession::GetPeerIdLength()
{
    return 16;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ��RCS�ͻ��˵�peer-id
 * ��  ��:
 * ����ֵ: ��RCS�ͻ��˵�peer-id
 * ��  ��:
 *   ʱ�� 2013.12.23
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
const char* PpsSession::GetLocalPeerId()
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
 *   ʱ�� 2013.12.23
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsSession::ReportResource()
{
	// �������ļ���ȡ����IP:PORT
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

	// �������ļ���ȡPps��Դ�ļ��ı���·��
	std::string save_path;
	GET_RCS_CONFIG_STR("pps.common.save-path", save_path);

    LOG(INFO) << "Report file resource :";
    PpsReportResourceMsg msg;

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
void PpsSession::SendKeepAlive()
{
	PpsKeepAliveMsg keep_alive_msg;
    Communicator::GetInstance().SendMsg(keep_alive_msg);
}


/*------------------------------------------------------------------------------
 * ��  ��: ��UGS�����Ϻ�Ļص�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsSession::OnUgsConnected()
{
    ReportResource();
}

}  // namespace BroadCache
