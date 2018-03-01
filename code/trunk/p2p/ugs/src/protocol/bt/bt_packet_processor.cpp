/*##############################################################################
 * 文件名   : bt_packet_processor.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : bt报文的处理类的实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "bt_packet_processor.hpp"
#include <algorithm>
#include <boost/regex.hpp>
#include "raw_data.hpp"
#include "bc_util.hpp"
#include "bc_assert.hpp"
#include "net_byte_order.hpp"
#include "bencode_entry.hpp"
#include "bencode.hpp"
#include "bt_session.hpp"
#include <glog/logging.h>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 编码回复以http get方式向tracker请求peer后返回的peer-list
 * 参  数: [in] peer以http get方式向tracker发送的请求信息
 *         [in] peer_list 返回的peer-list
 * 返回值: 编码后的字符串
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static std::string EncodePeerResponse(const BtGetPeerRequest& request, const std::vector<EndPoint>& peer_list)
{
    BencodeEntry entry;
    entry["complete"] = 1;
    entry["downloaded"] = 1; 
    entry["incomplete"] = 1;
    entry["interval"] = 180;
    entry["min interval"] = 60;
    
    if (request.compact)
    {
        char buf[1024];
        NetSerializer serializer(buf);
        FOREACH(peer, peer_list)
        {
            serializer & uint32(peer.ip) & uint16(peer.port);
        }
        entry["peers"] = std::string(buf, serializer.value());
    }
    else
    {
        auto& l = entry["peers"].List();
        FOREACH(peer, peer_list)
        {
            l.push_back(BencodeEntry());
            l.back()["ip"] = peer.ip;
            l.back()["port"] = peer.port;
        }
    }

    return Bencode(entry);
}

/*------------------------------------------------------------------------------
 * 描  述: 从http get请求中解析出相关参数
 * 参  数: [in] http_request http请求的数据
 *         [in] resolver 原始数据解析器
 * 返回值: 解析后的以http方式获取peer的请求
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static BtGetPeerRequest ParseHttpGetPeerRequest(const std::string& http_request,
                                                const RawDataResolver& resolver)
{
    BtGetPeerRequest request;

    // 获取url中的key = value对
    std::map<std::string, std::string> pairs = ParseKeyValuePairs(http_request);

    // 设置info-hash
    auto i = pairs.find("info_hash");
    BC_ASSERT(i != pairs.end());
    request.info_hash = FromHexUrl(i->second); 

    // 设置ip
    i = pairs.find("ip");
    request.requester.ip = ((i == pairs.end()) ?
        NetToHost(resolver.GetIpHeader()->saddr)
      : to_address_ul(i->second));

    // 设置port
    i = pairs.find("port");
    request.requester.port = ((i == pairs.end()) ? 
        NetToHost(resolver.GetTransportSourcePort())
      : boost::lexical_cast<decltype(request.requester.port)>(i->second));

    // 获取downloaded（peer已经下载的数据字节数）
    i = pairs.find("downloaded");
    uint64 downloaded = ((i == pairs.end()) ? 0
        : boost::lexical_cast<decltype(request.file_size)>(i->second));

    // 获取left（peer还需要下载的数据字节数）
    i = pairs.find("left");
    uint64 left = ((i == pairs.end()) ? 0
        : boost::lexical_cast<decltype(request.file_size)>(i->second));

    // 设置请求的文件大小
    request.file_size = downloaded + left;

    // 设置compact
    i = pairs.find("compact");
    request.compact = ((i == pairs.end()) ? false
        : (boost::lexical_cast<uint32>(i->second) != 0));
    
    // 设置num_want
    i = pairs.find("numwant");
    request.num_want = ((i == pairs.end()) ? -1
        : boost::lexical_cast<decltype(request.num_want)>(i->second));

    // 设置event
    i = pairs.find("event");
    if (i != pairs.end())
    {
        request.event = i->second;
    }

    // 设置tracker
    boost::smatch what;
    static const boost::regex kRegexHost("(?:\\s|\\S)*Host:\\s*(\\S+)(?:\\s|\\S)*");
    if (boost::regex_match(http_request, what, kRegexHost))
    {
        request.tracker = "http://" + what[1] + "/announce";
    }

    // 设置time
    request.time = time(nullptr);

    return request;
}

/*------------------------------------------------------------------------------
 * 描  述: 处理接收到peer以http get方式向tracker发起的请求
 * 参  数: [in] data 数据包数据
 *         [in] length 数据包长度
 * 返回值: 此数据包是否已经处理
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtHttpGetPeerProcessor::Process(const void* data, uint32 length)
{
    LOG(INFO) << "BtHttpGetPeerProcessor.Process(" << data << ", " << length << ")";

    static const char kMatch[] = "GET /announce?info_hash=";
    static const uint32 kMatchLength = sizeof(kMatch) / sizeof(kMatch[0]) - 1;  // 不匹配最后一个'\0'

    RawDataResolver resolver(data, length, RawDataResolver::FROM_ETHERNET);
    if (resolver.GetTransportProtocol() != IPPROTO_TCP)
        return false;

    char* tcp_data = (char*)(resolver.GetTransportData());  // tcp数据
    uint32 tcp_data_length = resolver.GetTransportDataLength();  // tcp数据长度

    // 判断报文是否匹配
    if (strncmp(tcp_data, kMatch, std::min(tcp_data_length, kMatchLength)) != 0)
        return false;

    LOG(INFO) << "Http get peer-list request received";
    LOG(INFO) << std::string(tcp_data, tcp_data_length);

    // 解析get请求中的参数
    BtGetPeerRequest request = 
        ParseHttpGetPeerRequest(std::string(tcp_data, tcp_data_length), resolver);

    LOG(INFO) << "Request infomation : ";
    LOG(INFO) << "info-hash : " << ToHex(request.info_hash);
    LOG(INFO) << "requester : " << request.requester;
    LOG(INFO) << "tracker : " << request.tracker;
    LOG(INFO) << "event : " << request.event;

    // 增加一条tracker记录
    bt_session_->GetHttpTrackerManager().AddTracker(request.info_hash,
        EndPoint(NetToHost(resolver.GetIpHeader()->daddr),
                 NetToHost(resolver.GetTransportDestPort())));

	// 进行热点资源判断，不是热点资源不进行重定向
	if (!bt_session_->IsHotResource(request.info_hash, request.requester)) return true;
	LOG(INFO) << "Hot Resource redirect";

    // 增加数据包的统计信息
    bt_session_->GetPacketStatistics().GetCounter(BT_HTTP_GET_PEER).Increase();

    static const boost::regex kRegexStopEvent("[Ss][Tt][Oo][Pp][Pp][Ee][Dd]");
    if (boost::regex_match(request.event, kRegexStopEvent))  // 是否是stopped事件
        return true;

    // 增加一条请求记录
    while (get_peer_requests_.size() > kBtGetPeerRequestMax)
    {
        get_peer_requests_.pop_front();
    }
    get_peer_requests_.push_back(request);
		
    // 编码返回的peer-list
    std::vector<EndPoint> peer_list = bt_session_->GetLocalPeers(request.info_hash);

    std::string http_data = EncodePeerResponse(request, peer_list);
    LOG(INFO) << "Reply peer-list : ";
    FOREACH(peer, peer_list)
    {
        LOG(INFO) << peer;
    }
    LOG(INFO) << "Response to http get peer-list request : ";
    LOG(INFO) << http_data;

    static const char* kHttpResponseHead = "HTTP/1.0 200 OK\r\n"
                                            "Content-Type: text/plain\r\n"
                                            "Content-Length: %d\r\n\r\n";

    char buf[4096] = {0};
    RawDataConstructor constructor(buf, data, length); 
    char* user_data = reinterpret_cast<char*>(constructor.GetTransportData());
    char* p = user_data; 

    // 设置http头部
    int bytes_formatted = sprintf(p, kHttpResponseHead, http_data.size());
    p += bytes_formatted;

    // 设置http数据
    memcpy(p, http_data.c_str(), http_data.size());
    p += http_data.size();

    uint32 user_data_length = p - user_data;
    constructor.SetTransportDataLength(user_data_length);

    // 用原始套接字发送回复
    if (RawDataSender::instance().Send(buf, p - buf))
    {
        LOG(INFO) << "Success to redirect http get peer request.";
        bt_session_->GetPacketStatistics().\
            GetCounter(BT_HTTP_GET_PEER_REDIRECT).Increase();
    }

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 处理peer以udp方式向tracker获取peer-list的请求
 * 参  数: [in] data 数据包的数据
 *         [in] length 数据包的长度
 * 返回值: 此数据包是否已经处理
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtUdpGetPeerProcessor::Process(const void* data, uint32 length)
{
    LOG(INFO) << "BtUdpGetPeerProcessor.Process(" << data << ", " << length << ")";

    RawDataResolver resolver(data, length, RawDataResolver::FROM_ETHERNET); 
    if (resolver.GetTransportProtocol() != IPPROTO_UDP)
        return false;

    char* udp_data = (char*)(resolver.GetTransportData());
    uint32 udp_data_length = resolver.GetTransportDataLength();
    LOG(INFO) << "udp data length : " << udp_data_length;

    if ((NetToHost<int32>(udp_data + 8) != 1)  // action是否是announce
        || (NetToHost<int32>(udp_data + 80) > 3))  // event是否合法
       return false; 

    LOG(INFO) << "Received udp get peer-list request.";
    LOG(INFO) << "Source port : " << NetToHost<uint16>(resolver.GetTransportSourcePort());
    LOG(INFO) << "Dest port : " << NetToHost<uint16>(resolver.GetTransportDestPort());
    LOG(INFO) << "info-hash : " << ToHex(std::string(udp_data + 16, udp_data + 36));

	std::string info_hash(udp_data + 16, udp_data + 36);
	EndPoint ep(NetToHost(resolver.GetIpHeader()->daddr), NetToHost(resolver.GetTransportDestPort()));

    // 增加一条udp tracker记录
    bt_session_->GetUdpTrackerManager().AddTracker(info_hash, ep);

	// 进行热点资源判断，不是热点资源不进行重定向
	
	if (!bt_session_->IsHotResource(info_hash, ep)) return true;
	LOG(INFO) << "Hot Resource redirect";

    // 增加一条数据包的统计信息
    bt_session_->GetPacketStatistics().GetCounter(BT_UDP_GET_PEER).Increase();

	

    char buf[1024];  // 回复数据缓冲区
    RawDataConstructor constructor(buf, data, length);

    char* p = (char*)(constructor.GetTransportData()); 
    NetSerializer serializer(p);
    
    serializer & int32(1)  // action = 1, announce 
        & NetToHost<int32>(udp_data + 12)  // transaction_id
        & int32(15)  // interval
        & int32(1024)  // leechers
        & int32(1024);  // seeders
    
    // 获取内网peer
    std::vector<EndPoint> peer_list = 
        bt_session_->GetLocalPeers(std::string(udp_data + 16, udp_data + 36)); 

    LOG(INFO) << "Reply peer-list : ";
    FOREACH(peer, peer_list)
    {
        LOG(INFO) << peer;
    }

    FOREACH(peer, peer_list)
    {
        serializer & (int32)(peer.ip) & (int16)(peer.port);  // ip and port
    }

    constructor.SetTransportDataLength(serializer.value() - p);

    // 用原始套接字发送回复
    if (RawDataSender::instance().Send(buf, serializer.value() - buf))
    {
        LOG(INFO) << "Success to redirect udp get peer-list request.";
        LOG(INFO) << "Data length : " << serializer.value() - buf;

        bt_session_->GetPacketStatistics().\
            GetCounter(BT_UDP_GET_PEER_REDIRECT).Increase();
    }

    return true;
}   

/*------------------------------------------------------------------------------
 * 描  述: 处理peer以dht方式peer-list的请求
 * 参  数: [in] data 数据包的数据
 *         [in] length 数据包的长度
 * 返回值: 此数据包是否已经处理
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtDhtGetPeerProcessor::Process(const void* data, uint32 length)
{
    LOG(INFO) << "BtDhtGetPeerProcessor.Process(" << data << ", " << length << ")";

    return false;
}

/*------------------------------------------------------------------------------
 * 描  述: 处理peer发送的handshake消息
 * 参  数: [in] data 数据包的数据
 *         [in] length 数据包的长度
 * 返回值: 此数据包是否已经处理
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtHandshakeProcessor::Process(const void* data, uint32 length)
{
    LOG(INFO) << "BtHandshakeProcessor.Process(" << data << ", " << length << ")";

    static const char kBtProtocol[] = "BitTorrent protocol";  // bt handshake字符串
    static const uint32 kBtProtocolLength = sizeof(kBtProtocol) / sizeof(char) - 1;

    RawDataResolver resolver(data, length, RawDataResolver::FROM_ETHERNET); 
    if (resolver.GetTransportProtocol() != IPPROTO_TCP)
        return false;

    char* tcp_data = (char*)(resolver.GetTransportData());

    uint32 tcp_data_length = resolver.GetTransportDataLength();

    if ((tcp_data_length <= kBtProtocolLength)  // 长度是否合法
        || (tcp_data[0] != kBtProtocolLength)  // 握手的协议字符串长度是否匹配
        || (strncmp(tcp_data + 1, kBtProtocol, kBtProtocolLength) != 0))  // 握手的协议字符串是否匹配
    {
        return false;
    }

    uint32 src_ip = resolver.GetSourceIp_r();
    uint32 dest_ip = resolver.GetDestIp_r();

    // 判断是否是RCS发出/收到的handshake报文
    std::vector<EndPoint> rcs = bt_session_->GetAllRcs();
    FOREACH(e, rcs)
    {
        if ((e.ip == src_ip) || (e.ip == dest_ip))
        {
            LOG(INFO) << "Handshake message from/to RCS, ignored.";
            return true;
        }
    }

    LOG(INFO) << "Bt handshake packet received.";
    char* q = tcp_data + 1 + kBtProtocolLength + 8;
    LOG(INFO) << "info-hash : " << ToHex(std::string(q, q + 20));

    // 增加一条数据包的统计信息
    bt_session_->GetPacketStatistics().GetCounter(BT_HANDSHAKE).Increase();

    static const uint32 kTransportDataLength = 6;

    char buf[256] = {0};
    RawDataConstructor constructor(buf, data, length);
    constructor.SetTransportDataLength(kTransportDataLength);

    // 用原始套接字发送回复数据
    if (RawDataSender::instance().Send(buf, 
        (char*)(constructor.GetTransportData()) - buf + kTransportDataLength))
    {
        LOG(INFO) << "Sucess to redirect bt handshake message.\n";
        bt_session_->GetPacketStatistics().GetCounter(BT_HANDSHAKE_REDIRECT).Increase();
    }
    
    return true;
}
    
}  // namespace BroadCache
