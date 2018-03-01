/*#############################################################################
 * 文件名   : bt_tracker_request.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月30日
 * 文件描述 : BtTrackerRequest声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
	//"&peer_id=%s" //使用此字段后向tracker请求peer-list会失败
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
 * 描  述: 构造函数
 * 参  数: 略
 * 返回值:
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
BtTrackerRequest::BtTrackerRequest(Session& session, const TorrentSP& torrent,
	const EndPoint& tracker_address)
	: TrackerRequest(tracker_address)
	, session_(session)
	, torrent_(torrent)
{
	// annouce请求同时也承担着将本端监听端口发布出去的使命，对于同时监听多个
	// 端口的情况，要考虑怎样将所有的监听端口都发布出去
	Session::ListenAddrVec addr_vec = session_.GetListenAddr();
	BC_ASSERT(!addr_vec.empty());

	//TODO: 这里暂时固定选第一个地址填充
	local_address_ = addr_vec.front();
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建连接
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
SocketConnectionSP BtTrackerRequest::CreateConnection()
{
	SocketConnectionSP sock_conn(new TcpSocketConnection(session_.sock_ios(), tracker_address()));
	return sock_conn;
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建发送请求
 * 参  数: [out] send_buf 构造的发送报文
 * 返回值: 成功/失败
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::ConstructSendRequest(SendBuffer& send_buf)
{
	LOG(INFO) << "Construct BT tracker request | " << tracker_address();

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	//TODO: sprintf调用不安全
	uint64 len = sprintf(send_buf.buf, 
		                 BT_TRACKER_ANNOUNCE_TEMPLATE,
		                 ToHexUrl(torrent->info_hash()->raw_data()).c_str(),
		                 local_address_.port,
		                 to_string(local_address_).c_str()); // HTTP头部Host字段
	send_buf.len = len;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理请求响应
 * 参  数: [in] error 错误码
 *         [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::ProcessResponse(const char* buf, uint64 len)
{
	// 检查HTTP响应头
	std::string response(buf, len);

	if (response.find("200 OK") == std::string::npos)
	{
		LOG(ERROR) << "Unexpected HTTP response code";
		return false;
	}

	// 解析响应内容
	if (!ParseTrackerResponse(response))
	{
		LOG(ERROR) << "Fail to parse tracker response";
		return false;
	}

	return false;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析Tracker返回内容
 * 参  数: [in] response 响应数据
 * 返回值: 成功/失败
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
bool BtTrackerRequest::ParseTrackerResponse(const std::string& response)
{
	// 从d8开始解析
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
 * 描  述: 将解析到的peer添加到policy中
 * 参  数: [in] peers 解析的peer
 * 返回值: 
 * 修  改:
 * 时间 2013年11月21日
 * 作者 teck_zhou
 * 描述 创建
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
 * 描  述: Regular返回的解析函数
 * 参  数: [in] e lazy entry
 * 返回值: 成功/失败
 * 修  改: 
 * 时间 2013年08月26日
 * 作者 tom_liu
 * 描述 创建
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

	// 解析"peers"
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
 * 描  述: Tracker返回信息peers对应的是string方式
 * 参  数:
 * 返回值: bool
 * 修  改: 
 * 时间 2013年08月26日
 * 作者 tom_liu
 * 描述 创建
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
