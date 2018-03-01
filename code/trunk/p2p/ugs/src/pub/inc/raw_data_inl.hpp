/*##############################################################################
 * 文件名   : raw_data_inl.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : RawDataResolver类的实现文件
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_RAW_DATA_INL
#define HEADER_RAW_DATA_INL

#include "net_byte_order.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: RawDataResolver类的构造函数
 * 参  数: [in] data 待解析的原始数据
 *         [in] length 待解析的原始数据长度
 *         [in] mode 原始数据的解析模式
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 *        不管以什么方式解析，应该总是保证
 *        1.data_成员指向原始数据的IP头部
 *        2.length_成员总是以太层数据长度
 -----------------------------------------------------------------------------*/ 
inline RawDataResolver::RawDataResolver(const void* data, uint32 length, ResolveMode mode)
      : data_((char*)(data) + ((mode == FROM_ETHERNET) ? kEtherHeaderLength : 0)),
        length_(length - (mode == FROM_ETHERNET ? kEtherHeaderLength : 0))
{
}
 
/*------------------------------------------------------------------------------
 * 描  述: 获取IP头部
 * 参  数:
 * 返回值: IP头部
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline iphdr* RawDataResolver::GetIpHeader() const
{
    return (iphdr*)(data_);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取TCP头部
 * 参  数:
 * 返回值: TCP头部
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline tcphdr* RawDataResolver::GetTcpHeader() const
{
    return (tcphdr*)(GetIpData()); 
}

/*------------------------------------------------------------------------------
 * 描  述: 获取UDP头部
 * 参  数:
 * 返回值: UDP头部
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline udphdr* RawDataResolver::GetUdpHeader() const
{
    return (udphdr*)(GetIpData());
}
 
/*------------------------------------------------------------------------------
 * 描  述: 获取以太层数据
 * 参  数:
 * 返回值: 以太层数据
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline void* RawDataResolver::GetEthernetData() const
{
    return data_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取IP数据
 * 参  数:
 * 返回值: IP数据
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline void* RawDataResolver::GetIpData() const
{
    return data_ + GetIpHeaderLength();
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
inline void* RawDataResolver::GetTransportData() const
{
    return data_ + GetIpHeaderLength() + GetTransportHeaderLength();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取以太头部长度
 * 参  数:
 * 返回值: 以太头部长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetEthernetHeaderLength() const
{
    return kEtherHeaderLength;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取IP头部长度
 * 参  数:
 * 返回值: IP头部长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetIpHeaderLength() const
{
    return GetIpHeader()->ihl * 4;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层头部长度
 * 参  数:
 * 返回值: 传输层头部长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetTransportHeaderLength() const
{
    switch (GetTransportProtocol())
    {
    case IPPROTO_TCP : return GetTcpHeader()->doff * 4;
    case IPPROTO_UDP : return kUdpHeaderLength;
    default : return 0;
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 获取以太层数据长度
 * 参  数:
 * 返回值: 以太层数据长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetEthernetDataLength() const
{
    return length_;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取IP层数据长度
 * 参  数:
 * 返回值: IP层数据长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetIpDataLength() const
{
    return GetEthernetDataLength() - GetIpHeaderLength();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层数据长度
 * 参  数:
 * 返回值: 传输层数据长度
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetTransportDataLength() const
{
    return GetIpDataLength() - GetTransportHeaderLength();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层协议
 * 参  数:
 * 返回值: 传输层协议
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint8 RawDataResolver::GetTransportProtocol() const
{
    return GetIpHeader()->protocol;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层源端口号（网络字节顺序）
 * 参  数:
 * 返回值: 传输层源端口号（网络字节顺序）
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint16 RawDataResolver::GetTransportSourcePort() const
{
    switch (GetTransportProtocol())
    {
    case IPPROTO_TCP : return GetTcpHeader()->source;
    case IPPROTO_UDP : return GetUdpHeader()->source;
    default : return 0;
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层源端口号（主机字节顺序）
 * 参  数:
 * 返回值: 传输层源端口号（主机字节顺序）
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint16 RawDataResolver::GetTransportSourcePort_r() const
{
    return NetToHost(GetTransportSourcePort());
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层目的端口号（网络字节顺序）
 * 参  数:
 * 返回值: 传输层目的端口号（网络字节顺序）
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint16 RawDataResolver::GetTransportDestPort() const
{
    switch (GetTransportProtocol())
    {
    case IPPROTO_TCP : return GetTcpHeader()->dest;
    case IPPROTO_UDP : return GetUdpHeader()->dest;
    default : return 0;
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 获取传输层目的端口号（主机字节顺序）
 * 参  数:
 * 返回值: 传输层目的端口号（主机字节顺序）
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint16 RawDataResolver::GetTransportDestPort_r() const
{
    return GetTransportDestPort();
}

/*------------------------------------------------------------------------------
 * 描  述: 获取IP层源地址（网络字节顺序）
 * 参  数:
 * 返回值: IP层源地址（网络字节顺序）
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetSourceIp() const
{
    return GetIpHeader()->saddr;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取IP层源地址（主机字节顺序）
 * 参  数:
 * 返回值: IP层源地址（主机字节顺序）
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetSourceIp_r() const
{
    return NetToHost(GetSourceIp());
}

/*------------------------------------------------------------------------------
 * 描  述: 获取IP层目的地址（网络字节顺序）
 * 参  数:
 * 返回值: IP层目的地址（网络字节顺序）
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetDestIp() const
{
    return GetIpHeader()->daddr;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取IP层目的地址（主机字节顺序）
 * 参  数:
 * 返回值: IP层目的地址（主机字节顺序）
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline uint32 RawDataResolver::GetDestIp_r() const
{
    return NetToHost(GetDestIp());
}

/*------------------------------------------------------------------------------
 * 描  述: 获取源地址
 * 参  数:
 * 返回值: 源地址
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline EndPoint RawDataResolver::GetSourceEndpoint() const
{
    return EndPoint(GetSourceIp_r(), GetTransportSourcePort_r());
}

/*------------------------------------------------------------------------------
 * 描  述: 获取目的地址 
 * 参  数:
 * 返回值: 目的地址
 * 修  改:
 *   时间 2013.11.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline EndPoint RawDataResolver::GetDestEndpoint() const
{
    return EndPoint(GetDestIp_r(), GetTransportDestPort_r());
}

}  // namespace BroadCache

#endif  // HEADER_RAW_DATA_INL
