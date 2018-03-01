/*##############################################################################
 * 文件名   : raw_data.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : 声明了RawDataResolver RawDataConstructor RawDataSender三个类
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_RAW_DATA
#define HEADER_RAW_DATA

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include "bc_typedef.hpp"
#include "singleton.hpp"
#include "endpoint.hpp"

namespace BroadCache
{

static const uint32 kEtherHeaderLength = sizeof(ether_header);  // 以太头部长度
static const uint32 kIpHeaderMinLength = sizeof(iphdr);  // IP头部最小长度
static const uint32 kTcpHeaderMinLength = sizeof(tcphdr);  // TCP头部最小长度
static const uint32 kUdpHeaderLength = sizeof(udphdr);  // UDP头部长度

// IP伪首部，用于计算校验和
#pragma pack(1)
struct PseudoHeader
{
    uint32 source_ip = 0;  // 源IP地址
    uint32 dest_ip = 0;  // 目的IP地址
    uint8 user = 0;  // 未知
    uint8 protocol = 0;  // 传输层协议
    uint16 length = 0;  // IP层数据长度
};
#pragma pack()

/*******************************************************************************
 *描  述: 网络原始数据的解析类
 *作  者: rosan
 *时  间: 2013.11.23
 ******************************************************************************/
class RawDataResolver
{
public:
    // 解析模式（确定解析的起点）
    enum ResolveMode 
    { 
        FROM_ETHERNET,  // 从以太头部开始解析
        FROM_IP,  // 从IP头部开始解析
    };

    inline RawDataResolver(const void* data, uint32 length, ResolveMode mode);
    
    inline iphdr* GetIpHeader() const;  // 获取IP头部
    inline tcphdr* GetTcpHeader() const;  // 获取TCP头部
    inline udphdr* GetUdpHeader() const;  // 获取UDP头部
    
    inline uint32 GetEthernetHeaderLength() const;  // 获取以太层头部长度
    inline uint32 GetIpHeaderLength() const;  // 获取IP头部长度
    inline uint32 GetTransportHeaderLength() const;  // 获取传输层头部长度

    inline void* GetEthernetData() const;  // 获取以太层数据
    inline void* GetIpData() const;  // 获取IP层数据
    inline void* GetTransportData() const;  // 获取传输层数据

    inline uint32 GetEthernetDataLength() const;  // 获取以太层数据长度
    inline uint32 GetIpDataLength() const;  // 获取IP层数据长度
    inline uint32 GetTransportDataLength() const;  // 获取传输层数据长度

    inline uint32 GetSourceIp() const;  // 获取IP层源地址（网络字节顺序）
    inline uint32 GetSourceIp_r() const;  // 获取IP层源地址（主机字节顺序）
    inline uint32 GetDestIp() const;  // 获取IP层目的地址（网络字节顺序）
    inline uint32 GetDestIp_r() const;  // 获取IP层目的地址（主机字节顺序）

    inline uint8 GetTransportProtocol() const;  // 获取传输层协议
    inline uint16 GetTransportSourcePort() const;  // 获取传输层源端口（网络字节顺序）
    inline uint16 GetTransportSourcePort_r() const;  // 获取传输层源端口（主机字节顺序）
    inline uint16 GetTransportDestPort() const;  // 获取传输层目的端口（网络字节顺序）
    inline uint16 GetTransportDestPort_r() const;  // 获取传输层目的端口（主机字节顺序）

    inline EndPoint GetSourceEndpoint() const;  // 获取源地址
    inline EndPoint GetDestEndpoint() const;  // 获取目的地址

    PseudoHeader GetPseudoHeader() const;  // 获取伪首部

private:
    char* data_ = nullptr;  // IP头部
    uint32 length_ = 0;  // 以太层数据长度（IP头部长度 + IP数据长度）
};

/*******************************************************************************
 *描  述: 网络原始数据的构造类
 *作  者: rosan
 *时  间: 2013.11.23
 ******************************************************************************/
class RawDataConstructor
{
public:
    RawDataConstructor(void* data, const void* template_data, uint32 template_length);
    
    void* GetTransportData() const;  // 获取传输层数据
    void SetTransportDataLength(uint32 length) const;  // 设置传输层数据长度

private:
    void* data_ = nullptr;  // IP头部
};

namespace Detail
{

/*******************************************************************************
 *描  述: 原始套接字类
 *作  者: rosan
 *时  间: 2013.11.23
 ******************************************************************************/
class RawDataSender
{
public:
    bool Init(const std::string& interface);  // 初始化原始套接字的网络接口

    bool Send(const void* data, uint32 length);  // 发送原始数据（从IP头部开始）

private:
    int fd_ = -1;  // 原始套接字对应的文件描述符
};

}  // namespace Detail

typedef Singleton<Detail::RawDataSender> RawDataSender;

}  // namespace BroadCache

#include "raw_data_inl.hpp"

#endif  // HEADER_RAW_DATA
