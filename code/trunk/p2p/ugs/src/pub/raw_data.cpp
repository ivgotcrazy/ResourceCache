/*##############################################################################
 * 文件名   : raw_data.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : RawDataConstructor RawDataSender 两个类的实现类 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "raw_data.hpp"
#include <sys/socket.h>
#include "net_byte_order.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 获取伪首部
 * 参  数:
 * 返回值: 伪首部
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 计算数据的和
 * 参  数: [in] data 数据指针
 *         [in] length 数据长度
 * 返回值: 数据的和
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 *        算法如下：
 *        将传入的数据拆成若干个2个字节的分组，
 *        如果传入的数据长度为奇数，则最后一字节单独为一组
 *        将拆分的各个分组的数字相加
 -----------------------------------------------------------------------------*/ 
static uint32 GetSum(const void* data, uint32 length)
{
    uint32 checksum = 0;  // 和

    const uint16* p = reinterpret_cast<const uint16*>(data);
    while (length > 1)  // 还可以分组
    {
        checksum += *(p++);
        length -= sizeof(uint16);
    }

    if (length != 0)  // 传入的数据长度为奇数
    {
        checksum += *reinterpret_cast<const uint8*>(p);
    }
    
    return checksum;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算校验和
 * 参  数: [in] sum 数据和
 * 返回值: 校验和
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static uint16 GetChecksum(uint32 sum)
{
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return static_cast<uint16>(~sum);
}

/*------------------------------------------------------------------------------
 * 描  述: 计算IP头部校验和
 * 参  数: [in] data 待校验的数据
 *         [in] length 待校验的数据长度
 * 返回值: IP头部校验和
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static uint16 GetIpHeaderChecksum(const void* data, uint32 length)
{
    return GetChecksum(GetSum(data, length));
}

/*------------------------------------------------------------------------------
 * 描  述: 计算IP数据的校验和
 * 参  数: [in] data 待校验的数据
 *         [in] length 待校验的数据长度
 *         [in] pseudo_header 待校验的伪首部
 * 返回值: IP数据的校验和
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 初始化原始套接字
 * 参  数: [in] interface 网卡接口
 * 返回值: 初始化是否成功
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool RawDataSender::Init(const std::string& interface)
{
    fd_ = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);  // 创建原始套接字
    if (fd_ < 0)
    {
        return false;
    }

    if (!interface.empty() && (interface.size() < IFNAMSIZ))
    {
        auto ifr = GetClearedStruct<ifreq>();
        strcpy(ifr.ifr_name, interface.c_str()); 

        if (ioctl(fd_, SIOCGIFHWADDR, &ifr) < 0)  // 设置网卡接口
        {
            return false;
        }
    }

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 发送原始数据
 * 参  数: [in] data 原始数据
 *         [in] length 原始数据长度
 * 返回值: 发送是否成功
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
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
 * 描  述: RawDataConstructor构造函数
 * 参  数: [out] data
 *         [in] template_data 数据模板（即以此数据为模板，构造一个回复报文）
 *         [in] template_length 模板数据长度
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
RawDataConstructor::RawDataConstructor(void* data, const void* template_data,
    uint32 template_length) : data_(data)
{
    RawDataResolver src_resolver(template_data, template_length, RawDataResolver::FROM_ETHERNET);
    
    // 拷贝IP和传输层头部
    memcpy(data, src_resolver.GetEthernetData(), 
        src_resolver.GetIpHeaderLength() + src_resolver.GetTransportHeaderLength());

    RawDataResolver dest_resolver(data, 1024, RawDataResolver::FROM_IP);

    // 设置IP头部
    iphdr* src_ip_header = src_resolver.GetIpHeader();
    iphdr* dest_ip_header = dest_resolver.GetIpHeader();

    dest_ip_header->saddr = src_ip_header->daddr;
    dest_ip_header->daddr = src_ip_header->saddr;
    dest_ip_header->ttl = 46;
    
    uint8 protocol = dest_resolver.GetTransportProtocol();
    if (protocol == IPPROTO_TCP)
    {
        // 设置TCP头部
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
        // 设置UDP头部
        udphdr* src_udp_header = src_resolver.GetUdpHeader();
        udphdr* dest_udp_header = dest_resolver.GetUdpHeader();
        
        dest_udp_header->source = src_udp_header->dest;
        dest_udp_header->dest = src_udp_header->source; 
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层数据
 * 参  数:
 * 返回值: 传输层数据
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void* RawDataConstructor::GetTransportData() const
{
    RawDataResolver resolver(data_, 1024, RawDataResolver::FROM_IP);
    return resolver.GetTransportData();
}

/*------------------------------------------------------------------------------
 * 描  述: 设置传输层数据长度
 * 参  数: [in] length 传输层数据长度
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void RawDataConstructor::SetTransportDataLength(uint32 length) const
{
    RawDataResolver resolver_org(data_, 1024, RawDataResolver::FROM_IP);
    RawDataResolver resolver(data_, resolver_org.GetIpHeaderLength()
        + resolver_org.GetTransportHeaderLength() + length, RawDataResolver::FROM_IP);
 
    // 设置IP头部校验和
    iphdr* ip_header = resolver.GetIpHeader();
    ip_header->tot_len = HostToNet<uint16>(resolver.GetEthernetDataLength());
    ip_header->check = 0;
    ip_header->check = GetIpHeaderChecksum(ip_header, resolver.GetIpHeaderLength());

    uint32 ip_data_length = resolver.GetIpDataLength();
    PseudoHeader pseudo_header = resolver.GetPseudoHeader();

    uint8 protocol = resolver.GetTransportProtocol();
    if (protocol == IPPROTO_TCP)
    {
        // 设置TCP头部校验和
        tcphdr* tcp_header = resolver.GetTcpHeader();
        tcp_header->check = 0;
        tcp_header->check = GetIpDataChecksum(tcp_header, ip_data_length, pseudo_header);
    }
    else if (protocol == IPPROTO_UDP)
    {
        // 设置UDP头部校验和
        udphdr* udp_header = resolver.GetUdpHeader();
        udp_header->len = HostToNet<uint16>(resolver.GetIpDataLength());
        udp_header->check = 0;
        udp_header->check = GetIpDataChecksum(udp_header, ip_data_length, pseudo_header);
    }
}

}  // namespace BroadCache
