/*##############################################################################
 * �ļ���   : raw_data.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : RawDataConstructor RawDataSender �������ʵ���� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "raw_data.hpp"
#include <sys/socket.h>
#include "net_byte_order.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡα�ײ�
 * ��  ��:
 * ����ֵ: α�ײ�
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
PseudoHeader RawDataResolver::GetPseudoHeader() const
{
    iphdr* ip_header = GetIpHeader();

    PseudoHeader pseudo_header;
    pseudo_header.source_ip = ip_header->saddr;
    pseudo_header.dest_ip = ip_header->daddr;
    pseudo_header.user = 0;
    pseudo_header.protocol = ip_header->protocol;
    pseudo_header.length = HostToNet<uint16>(GetIpDataLength());

    return pseudo_header; 
}

/*------------------------------------------------------------------------------
 * ��  ��: �������ݵĺ�
 * ��  ��: [in] data ����ָ��
 *         [in] length ���ݳ���
 * ����ֵ: ���ݵĺ�
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 *        �㷨���£�
 *        ����������ݲ�����ɸ�2���ֽڵķ��飬
 *        �����������ݳ���Ϊ�����������һ�ֽڵ���Ϊһ��
 *        ����ֵĸ���������������
 -----------------------------------------------------------------------------*/ 
static uint32 GetSum(const void* data, uint32 length)
{
    uint32 checksum = 0;  // ��

    const uint16* p = reinterpret_cast<const uint16*>(data);
    while (length > 1)  // �����Է���
    {
        checksum += *(p++);
        length -= sizeof(uint16);
    }

    if (length != 0)  // ��������ݳ���Ϊ����
    {
        checksum += *reinterpret_cast<const uint8*>(p);
    }
    
    return checksum;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����У���
 * ��  ��: [in] sum ���ݺ�
 * ����ֵ: У���
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static uint16 GetChecksum(uint32 sum)
{
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return static_cast<uint16>(~sum);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����IPͷ��У���
 * ��  ��: [in] data ��У�������
 *         [in] length ��У������ݳ���
 * ����ֵ: IPͷ��У���
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static uint16 GetIpHeaderChecksum(const void* data, uint32 length)
{
    return GetChecksum(GetSum(data, length));
}

/*------------------------------------------------------------------------------
 * ��  ��: ����IP���ݵ�У���
 * ��  ��: [in] data ��У�������
 *         [in] length ��У������ݳ���
 *         [in] pseudo_header ��У���α�ײ�
 * ����ֵ: IP���ݵ�У���
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static uint16 GetIpDataChecksum(const void* data, uint32 length, 
                                const PseudoHeader& pseudo_header)
{
    return GetChecksum(GetSum(data, length) 
        + GetSum(&pseudo_header, sizeof(PseudoHeader)));
}

namespace Detail
{

/*------------------------------------------------------------------------------
 * ��  ��: ��ʼ��ԭʼ�׽���
 * ��  ��: [in] interface �����ӿ�
 * ����ֵ: ��ʼ���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool RawDataSender::Init(const std::string& interface)
{
    fd_ = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);  // ����ԭʼ�׽���
    if (fd_ < 0)
    {
        return false;
    }

    if (!interface.empty() && (interface.size() < IFNAMSIZ))
    {
        auto ifr = GetClearedStruct<ifreq>();
        strcpy(ifr.ifr_name, interface.c_str()); 

        if (ioctl(fd_, SIOCGIFHWADDR, &ifr) < 0)  // ���������ӿ�
        {
            return false;
        }
    }

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����ԭʼ����
 * ��  ��: [in] data ԭʼ����
 *         [in] length ԭʼ���ݳ���
 * ����ֵ: �����Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool RawDataSender::Send(const void* data, uint32 length)
{
    RawDataResolver resolver(data, length, RawDataResolver::FROM_IP);

    auto addr = GetClearedStruct<sockaddr_in>();
    addr.sin_family = AF_INET;
    addr.sin_port = resolver.GetTransportDestPort();
    addr.sin_addr.s_addr = resolver.GetIpHeader()->daddr;
    
    bool ok = (sendto(fd_, data, length, 0, (struct sockaddr*)(&addr), sizeof(addr)) > 0);
    if (!ok)
    {
        LOG(INFO) << "Fail to send raw data : "  << strerror(errno);
    }

    return ok;
}

}  // namespace Detail

/*------------------------------------------------------------------------------
 * ��  ��: RawDataConstructor���캯��
 * ��  ��: [out] data
 *         [in] template_data ����ģ�壨���Դ�����Ϊģ�壬����һ���ظ����ģ�
 *         [in] template_length ģ�����ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
RawDataConstructor::RawDataConstructor(void* data, const void* template_data,
    uint32 template_length) : data_(data)
{
    RawDataResolver src_resolver(template_data, template_length, RawDataResolver::FROM_ETHERNET);
    
    // ����IP�ʹ����ͷ��
    memcpy(data, src_resolver.GetEthernetData(), 
        src_resolver.GetIpHeaderLength() + src_resolver.GetTransportHeaderLength());

    RawDataResolver dest_resolver(data, 1024, RawDataResolver::FROM_IP);

    // ����IPͷ��
    iphdr* src_ip_header = src_resolver.GetIpHeader();
    iphdr* dest_ip_header = dest_resolver.GetIpHeader();

    dest_ip_header->saddr = src_ip_header->daddr;
    dest_ip_header->daddr = src_ip_header->saddr;
    dest_ip_header->ttl = 46;
    
    uint8 protocol = dest_resolver.GetTransportProtocol();
    if (protocol == IPPROTO_TCP)
    {
        // ����TCPͷ��
        tcphdr* src_tcp_header = src_resolver.GetTcpHeader();
        tcphdr* dest_tcp_header = dest_resolver.GetTcpHeader();

        dest_tcp_header->source = src_tcp_header->dest;
        dest_tcp_header->dest = src_tcp_header->source;
        dest_tcp_header->seq = src_tcp_header->ack_seq;
        dest_tcp_header->ack_seq = HostToNet(NetToHost(src_tcp_header->seq)
            + src_resolver.GetTransportDataLength());

        dest_tcp_header->window = HostToNet<uint16>(6432);
        dest_tcp_header->syn = 0;
        dest_tcp_header->ack = 1;
        dest_tcp_header->fin = 1;
        dest_tcp_header->rst = 0;
        dest_tcp_header->psh = 1;
        dest_tcp_header->urg = 0;
    }
    else if (protocol == IPPROTO_UDP)
    {
        // ����UDPͷ��
        udphdr* src_udp_header = src_resolver.GetUdpHeader();
        udphdr* dest_udp_header = dest_resolver.GetUdpHeader();
        
        dest_udp_header->source = src_udp_header->dest;
        dest_udp_header->dest = src_udp_header->source; 
    }
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ���������
 * ��  ��:
 * ����ֵ: ���������
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void* RawDataConstructor::GetTransportData() const
{
    RawDataResolver resolver(data_, 1024, RawDataResolver::FROM_IP);
    return resolver.GetTransportData();
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ô�������ݳ���
 * ��  ��: [in] length ��������ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void RawDataConstructor::SetTransportDataLength(uint32 length) const
{
    RawDataResolver resolver_org(data_, 1024, RawDataResolver::FROM_IP);
    RawDataResolver resolver(data_, resolver_org.GetIpHeaderLength()
        + resolver_org.GetTransportHeaderLength() + length, RawDataResolver::FROM_IP);
 
    // ����IPͷ��У���
    iphdr* ip_header = resolver.GetIpHeader();
    ip_header->tot_len = HostToNet<uint16>(resolver.GetEthernetDataLength());
    ip_header->check = 0;
    ip_header->check = GetIpHeaderChecksum(ip_header, resolver.GetIpHeaderLength());

    uint32 ip_data_length = resolver.GetIpDataLength();
    PseudoHeader pseudo_header = resolver.GetPseudoHeader();

    uint8 protocol = resolver.GetTransportProtocol();
    if (protocol == IPPROTO_TCP)
    {
        // ����TCPͷ��У���
        tcphdr* tcp_header = resolver.GetTcpHeader();
        tcp_header->check = 0;
        tcp_header->check = GetIpDataChecksum(tcp_header, ip_data_length, pseudo_header);
    }
    else if (protocol == IPPROTO_UDP)
    {
        // ����UDPͷ��У���
        udphdr* udp_header = resolver.GetUdpHeader();
        udp_header->len = HostToNet<uint16>(resolver.GetIpDataLength());
        udp_header->check = 0;
        udp_header->check = GetIpDataChecksum(udp_header, ip_data_length, pseudo_header);
    }
}

}  // namespace BroadCache
