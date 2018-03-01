/*##############################################################################
 * �ļ���   : bt_packet_processor.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : bt���ĵĴ������ʵ���ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
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
 * ��  ��: ����ظ���http get��ʽ��tracker����peer�󷵻ص�peer-list
 * ��  ��: [in] peer��http get��ʽ��tracker���͵�������Ϣ
 *         [in] peer_list ���ص�peer-list
 * ����ֵ: �������ַ���
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ��http get�����н�������ز���
 * ��  ��: [in] http_request http���������
 *         [in] resolver ԭʼ���ݽ�����
 * ����ֵ: ���������http��ʽ��ȡpeer������
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static BtGetPeerRequest ParseHttpGetPeerRequest(const std::string& http_request,
                                                const RawDataResolver& resolver)
{
    BtGetPeerRequest request;

    // ��ȡurl�е�key = value��
    std::map<std::string, std::string> pairs = ParseKeyValuePairs(http_request);

    // ����info-hash
    auto i = pairs.find("info_hash");
    BC_ASSERT(i != pairs.end());
    request.info_hash = FromHexUrl(i->second); 

    // ����ip
    i = pairs.find("ip");
    request.requester.ip = ((i == pairs.end()) ?
        NetToHost(resolver.GetIpHeader()->saddr)
      : to_address_ul(i->second));

    // ����port
    i = pairs.find("port");
    request.requester.port = ((i == pairs.end()) ? 
        NetToHost(resolver.GetTransportSourcePort())
      : boost::lexical_cast<decltype(request.requester.port)>(i->second));

    // ��ȡdownloaded��peer�Ѿ����ص������ֽ�����
    i = pairs.find("downloaded");
    uint64 downloaded = ((i == pairs.end()) ? 0
        : boost::lexical_cast<decltype(request.file_size)>(i->second));

    // ��ȡleft��peer����Ҫ���ص������ֽ�����
    i = pairs.find("left");
    uint64 left = ((i == pairs.end()) ? 0
        : boost::lexical_cast<decltype(request.file_size)>(i->second));

    // ����������ļ���С
    request.file_size = downloaded + left;

    // ����compact
    i = pairs.find("compact");
    request.compact = ((i == pairs.end()) ? false
        : (boost::lexical_cast<uint32>(i->second) != 0));
    
    // ����num_want
    i = pairs.find("numwant");
    request.num_want = ((i == pairs.end()) ? -1
        : boost::lexical_cast<decltype(request.num_want)>(i->second));

    // ����event
    i = pairs.find("event");
    if (i != pairs.end())
    {
        request.event = i->second;
    }

    // ����tracker
    boost::smatch what;
    static const boost::regex kRegexHost("(?:\\s|\\S)*Host:\\s*(\\S+)(?:\\s|\\S)*");
    if (boost::regex_match(http_request, what, kRegexHost))
    {
        request.tracker = "http://" + what[1] + "/announce";
    }

    // ����time
    request.time = time(nullptr);

    return request;
}

/*------------------------------------------------------------------------------
 * ��  ��: ������յ�peer��http get��ʽ��tracker���������
 * ��  ��: [in] data ���ݰ�����
 *         [in] length ���ݰ�����
 * ����ֵ: �����ݰ��Ƿ��Ѿ�����
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtHttpGetPeerProcessor::Process(const void* data, uint32 length)
{
    LOG(INFO) << "BtHttpGetPeerProcessor.Process(" << data << ", " << length << ")";

    static const char kMatch[] = "GET /announce?info_hash=";
    static const uint32 kMatchLength = sizeof(kMatch) / sizeof(kMatch[0]) - 1;  // ��ƥ�����һ��'\0'

    RawDataResolver resolver(data, length, RawDataResolver::FROM_ETHERNET);
    if (resolver.GetTransportProtocol() != IPPROTO_TCP)
        return false;

    char* tcp_data = (char*)(resolver.GetTransportData());  // tcp����
    uint32 tcp_data_length = resolver.GetTransportDataLength();  // tcp���ݳ���

    // �жϱ����Ƿ�ƥ��
    if (strncmp(tcp_data, kMatch, std::min(tcp_data_length, kMatchLength)) != 0)
        return false;

    LOG(INFO) << "Http get peer-list request received";
    LOG(INFO) << std::string(tcp_data, tcp_data_length);

    // ����get�����еĲ���
    BtGetPeerRequest request = 
        ParseHttpGetPeerRequest(std::string(tcp_data, tcp_data_length), resolver);

    LOG(INFO) << "Request infomation : ";
    LOG(INFO) << "info-hash : " << ToHex(request.info_hash);
    LOG(INFO) << "requester : " << request.requester;
    LOG(INFO) << "tracker : " << request.tracker;
    LOG(INFO) << "event : " << request.event;

    // ����һ��tracker��¼
    bt_session_->GetHttpTrackerManager().AddTracker(request.info_hash,
        EndPoint(NetToHost(resolver.GetIpHeader()->daddr),
                 NetToHost(resolver.GetTransportDestPort())));

	// �����ȵ���Դ�жϣ������ȵ���Դ�������ض���
	if (!bt_session_->IsHotResource(request.info_hash, request.requester)) return true;
	LOG(INFO) << "Hot Resource redirect";

    // �������ݰ���ͳ����Ϣ
    bt_session_->GetPacketStatistics().GetCounter(BT_HTTP_GET_PEER).Increase();

    static const boost::regex kRegexStopEvent("[Ss][Tt][Oo][Pp][Pp][Ee][Dd]");
    if (boost::regex_match(request.event, kRegexStopEvent))  // �Ƿ���stopped�¼�
        return true;

    // ����һ�������¼
    while (get_peer_requests_.size() > kBtGetPeerRequestMax)
    {
        get_peer_requests_.pop_front();
    }
    get_peer_requests_.push_back(request);
		
    // ���뷵�ص�peer-list
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

    // ����httpͷ��
    int bytes_formatted = sprintf(p, kHttpResponseHead, http_data.size());
    p += bytes_formatted;

    // ����http����
    memcpy(p, http_data.c_str(), http_data.size());
    p += http_data.size();

    uint32 user_data_length = p - user_data;
    constructor.SetTransportDataLength(user_data_length);

    // ��ԭʼ�׽��ַ��ͻظ�
    if (RawDataSender::instance().Send(buf, p - buf))
    {
        LOG(INFO) << "Success to redirect http get peer request.";
        bt_session_->GetPacketStatistics().\
            GetCounter(BT_HTTP_GET_PEER_REDIRECT).Increase();
    }

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����peer��udp��ʽ��tracker��ȡpeer-list������
 * ��  ��: [in] data ���ݰ�������
 *         [in] length ���ݰ��ĳ���
 * ����ֵ: �����ݰ��Ƿ��Ѿ�����
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
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

    if ((NetToHost<int32>(udp_data + 8) != 1)  // action�Ƿ���announce
        || (NetToHost<int32>(udp_data + 80) > 3))  // event�Ƿ�Ϸ�
       return false; 

    LOG(INFO) << "Received udp get peer-list request.";
    LOG(INFO) << "Source port : " << NetToHost<uint16>(resolver.GetTransportSourcePort());
    LOG(INFO) << "Dest port : " << NetToHost<uint16>(resolver.GetTransportDestPort());
    LOG(INFO) << "info-hash : " << ToHex(std::string(udp_data + 16, udp_data + 36));

	std::string info_hash(udp_data + 16, udp_data + 36);
	EndPoint ep(NetToHost(resolver.GetIpHeader()->daddr), NetToHost(resolver.GetTransportDestPort()));

    // ����һ��udp tracker��¼
    bt_session_->GetUdpTrackerManager().AddTracker(info_hash, ep);

	// �����ȵ���Դ�жϣ������ȵ���Դ�������ض���
	
	if (!bt_session_->IsHotResource(info_hash, ep)) return true;
	LOG(INFO) << "Hot Resource redirect";

    // ����һ�����ݰ���ͳ����Ϣ
    bt_session_->GetPacketStatistics().GetCounter(BT_UDP_GET_PEER).Increase();

	

    char buf[1024];  // �ظ����ݻ�����
    RawDataConstructor constructor(buf, data, length);

    char* p = (char*)(constructor.GetTransportData()); 
    NetSerializer serializer(p);
    
    serializer & int32(1)  // action = 1, announce 
        & NetToHost<int32>(udp_data + 12)  // transaction_id
        & int32(15)  // interval
        & int32(1024)  // leechers
        & int32(1024);  // seeders
    
    // ��ȡ����peer
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

    // ��ԭʼ�׽��ַ��ͻظ�
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
 * ��  ��: ����peer��dht��ʽpeer-list������
 * ��  ��: [in] data ���ݰ�������
 *         [in] length ���ݰ��ĳ���
 * ����ֵ: �����ݰ��Ƿ��Ѿ�����
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtDhtGetPeerProcessor::Process(const void* data, uint32 length)
{
    LOG(INFO) << "BtDhtGetPeerProcessor.Process(" << data << ", " << length << ")";

    return false;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����peer���͵�handshake��Ϣ
 * ��  ��: [in] data ���ݰ�������
 *         [in] length ���ݰ��ĳ���
 * ����ֵ: �����ݰ��Ƿ��Ѿ�����
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtHandshakeProcessor::Process(const void* data, uint32 length)
{
    LOG(INFO) << "BtHandshakeProcessor.Process(" << data << ", " << length << ")";

    static const char kBtProtocol[] = "BitTorrent protocol";  // bt handshake�ַ���
    static const uint32 kBtProtocolLength = sizeof(kBtProtocol) / sizeof(char) - 1;

    RawDataResolver resolver(data, length, RawDataResolver::FROM_ETHERNET); 
    if (resolver.GetTransportProtocol() != IPPROTO_TCP)
        return false;

    char* tcp_data = (char*)(resolver.GetTransportData());

    uint32 tcp_data_length = resolver.GetTransportDataLength();

    if ((tcp_data_length <= kBtProtocolLength)  // �����Ƿ�Ϸ�
        || (tcp_data[0] != kBtProtocolLength)  // ���ֵ�Э���ַ��������Ƿ�ƥ��
        || (strncmp(tcp_data + 1, kBtProtocol, kBtProtocolLength) != 0))  // ���ֵ�Э���ַ����Ƿ�ƥ��
    {
        return false;
    }

    uint32 src_ip = resolver.GetSourceIp_r();
    uint32 dest_ip = resolver.GetDestIp_r();

    // �ж��Ƿ���RCS����/�յ���handshake����
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

    // ����һ�����ݰ���ͳ����Ϣ
    bt_session_->GetPacketStatistics().GetCounter(BT_HANDSHAKE).Increase();

    static const uint32 kTransportDataLength = 6;

    char buf[256] = {0};
    RawDataConstructor constructor(buf, data, length);
    constructor.SetTransportDataLength(kTransportDataLength);

    // ��ԭʼ�׽��ַ��ͻظ�����
    if (RawDataSender::instance().Send(buf, 
        (char*)(constructor.GetTransportData()) - buf + kTransportDataLength))
    {
        LOG(INFO) << "Sucess to redirect bt handshake message.\n";
        bt_session_->GetPacketStatistics().GetCounter(BT_HANDSHAKE_REDIRECT).Increase();
    }
    
    return true;
}
    
}  // namespace BroadCache
