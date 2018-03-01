/*#############################################################################
 * �ļ���   : bt_tracker_request.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��30��
 * �ļ����� : BtTrackerRequest����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bt_tracker_request.hpp"

#include "bc_assert.hpp"
#include "bc_util.hpp"
#include "lazy_entry.hpp"
#include "mem_buffer_pool.hpp"
#include "socket_connection.hpp"
#include "torrent.hpp"
#include "info_hash.hpp"
#include "session.hpp"
#include "policy.hpp"
#include "net_byte_order_impl_forward.hpp"
#include "ip_filter.hpp"
#include "bc_io.hpp"

namespace BroadCache
{

static const char* const BT_TRACKER_ANNOUNCE_TEMPLATE =  
	"GET /announce?"
	"info_hash=%s"
	//"&peer_id=%s" //ʹ�ô��ֶκ���tracker����peer-list��ʧ��
	"&port=%d"
	"&uploaded=212992"
	"&downloaded=5242880"
	"&left=1042485977"
	"&corrupt=0"
	"&compact=1"
	"&numwant=200"
	"&key=47D4A7FC"
	"&event=started"
	"&no_peer_id=1"
	" HTTP/1.1\r\n"
	"Host: %s\r\n"
	"Accept-Encoding: gzip\r\n"
	"Connection: Close\r\n\r\n";

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: ��
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
BtTrackerRequest::BtTrackerRequest(Session& session, const TorrentSP& torrent,
	const EndPoint& tracker_address)
	: TrackerRequest(tracker_address)
	, session_(session)
	, torrent_(torrent)
{
	// annouce����ͬʱҲ�е��Ž����˼����˿ڷ�����ȥ��ʹ��������ͬʱ�������
	// �˿ڵ������Ҫ�������������еļ����˿ڶ�������ȥ
	Session::ListenAddrVec addr_vec = session_.GetListenAddr();
	BC_ASSERT(!addr_vec.empty());

	//TODO: ������ʱ�̶�ѡ��һ����ַ���
	local_address_ = addr_vec.front();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
SocketConnectionSP BtTrackerRequest::CreateConnection()
{
	SocketConnectionSP sock_conn(new TcpSocketConnection(session_.sock_ios(), tracker_address()));
	return sock_conn;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������
 * ��  ��: [out] send_buf ����ķ��ͱ���
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::ConstructSendRequest(SendBuffer& send_buf)
{
	LOG(INFO) << "Construct BT tracker request | " << tracker_address();

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	//TODO: sprintf���ò���ȫ
	uint64 len = sprintf(send_buf.buf, 
		                 BT_TRACKER_ANNOUNCE_TEMPLATE,
		                 ToHexUrl(torrent->info_hash()->raw_data()).c_str(),
		                 local_address_.port,
		                 to_string(local_address_).c_str()); // HTTPͷ��Host�ֶ�
	send_buf.len = len;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������Ӧ
 * ��  ��: [in] error ������
 *         [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::ProcessResponse(const char* buf, uint64 len)
{
	// ���HTTP��Ӧͷ
	std::string response(buf, len);

	if (response.find("200 OK") == std::string::npos)
	{
		LOG(ERROR) << "Unexpected HTTP response code";
		return false;
	}

	// ������Ӧ����
	if (!ParseTrackerResponse(response))
	{
		LOG(ERROR) << "Fail to parse tracker response";
		return false;
	}

	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Tracker��������
 * ��  ��: [in] response ��Ӧ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::ParseTrackerResponse(const std::string& response)
{
	// ��d8��ʼ����
	auto iter = response.find("d8");
	if (iter == std::string::npos)
	{
		LOG(ERROR) << "Can't find 'd8'";
		return false;
	}

	LazyEntry entry;
	int ret = LazyBdecode(response.c_str() + iter, response.c_str() + response.size(), entry);
	if (ret != 0 || entry.Type() != LazyEntry::DICT_ENTRY)
	{
		LOG(ERROR) << "Fail to construct lazy entry";
		return false;
	}

	std::vector<EndPoint> peer_list;
	if (!Parse(entry, peer_list))
	{
		LOG(ERROR) << "Tracker response content error";
		return false;
	}

	AddPeers(peer_list);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������peer��ӵ�policy��
 * ��  ��: [in] peers ������peer
 * ����ֵ: 
 * ��  ��:
 * ʱ�� 2013��11��21��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
void BtTrackerRequest::AddPeers(std::vector<EndPoint> peers)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	IpFilter& filter = IpFilter::GetInstance();

	for (EndPoint& peer : peers)
	{
		AccessInfo info = filter.Access(to_address(peer.ip));

		if (info.flags == BLOCKED) continue;

		PeerType peer_type = (info.flags == INNER) ? PEER_INNER : PEER_OUTER;

		torrent->policy()->AddPeer(peer, peer_type, PEER_RETRIVE_TRACKER);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: Regular���صĽ�������
 * ��  ��: [in] e lazy entry
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��: 
 * ʱ�� 2013��08��26��
 * ���� tom_liu
 * ���� ����
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::Parse(LazyEntry const& e, std::vector<EndPoint>& peer_list)
{
	LazyEntry const* failure_entry = e.DictFindString("failure reason");
	if (failure_entry)
	{
		LOG(ERROR)<<"Failure reason:  " <<  failure_entry->StringValue() ;
		return false;
	}

	LazyEntry const* warning_entry = e.DictFindString("warning message");
	if (warning_entry)
	{
		LOG(ERROR)<<"Warning reason: "<< warning_entry->StringValue();
		return false;
	}
	
	LazyEntry const* peers_entry = e.DictFind("peers");
	if (!peers_entry)
	{
		LOG(WARNING) << "Fail to find 'peers' entry in response";
		return false;
	}

	// ����"peers"
	if (peers_entry->Type() == LazyEntry::STRING_ENTRY) // binary mode
	{
		char const* peers = peers_entry->StringPtr();
		int len = peers_entry->StringLength();
		for (int i = 0; i < len; i += 6)
		{
			if (len - i < 6) break;

			EndPoint peer;
			peer.ip = IO::read_uint32(peers);
			peer.port = IO::read_uint16(peers);

			LOG(INFO)<< "Retrived one binary peer from tracker | " << peer;

			peer_list.push_back(peer);
		}
	}
	else if (peers_entry->Type() == LazyEntry::LIST_ENTRY) // dictionary mode
	{
		int len = peers_entry->ListSize();
		for (int i = 0; i < len; ++i)
		{
			EndPoint peer;
			if (!ExtractPeerInfo(*peers_entry->ListAt(i), peer)) return false;

			LOG(INFO)<< "Retrived one string peer from tracker | " << peer;

			peer_list.push_back(peer);
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Tracker������Ϣpeers��Ӧ����string��ʽ
 * ��  ��:
 * ����ֵ: bool
 * ��  ��: 
 * ʱ�� 2013��08��26��
 * ���� tom_liu
 * ���� ����
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::ExtractPeerInfo(LazyEntry const& info, EndPoint& peer)
{
	// extract peer id (if any)
	if (info.Type() != LazyEntry::DICT_ENTRY)
	{
		LOG(ERROR) << "Invalid peer dict";
		return false;
	}

	// extract ip
	LazyEntry const* i = info.DictFindString("ip");
	if (i == 0)
	{
		LOG(ERROR) << "Can't find 'ip' in entry";
		return false;
	}
	peer.ip = to_address_ul(i->StringValue());

	// extract port
	i = info.DictFindInt("port");
	if (i == 0)
	{
		LOG(ERROR) << "Can't find 'port' in entry";
		return false;
	}
	peer.port = (unsigned short)i->IntValue();

	return true;
}

}
