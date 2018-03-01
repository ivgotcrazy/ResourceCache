/*##############################################################################
 * 文件名   : bt_types.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.19
 * 文件描述 : 此文件定义了bt协议模块的相关通用结构和类型
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_BT_TYPES
#define HEADER_BT_TYPES

#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

static const uint32 kBtGetPeerRequestMax = 1024;  // 保存的http get peer请求的最大个数

// bt数据包类型
enum BtPacketType
{
    BT_PACKET_TYPE_BEGIN = 0,  // 不要在此枚举之前定义任何枚举
    BT_COMMON_PACKET = BT_PACKET_TYPE_BEGIN,
    BT_HTTP_GET_PEER,  // http get方式请求peer-list包
    BT_UDP_GET_PEER,  // udp方式请求peer-list包
    BT_DHT_GET_PEER,  // 用DHT协议方式请求peer-list包
    BT_HANDSHAKE,  // bt协议的handshake包
    BT_HTTP_GET_PEER_REDIRECT,  // http get请求peer-list包的回复包
    BT_UDP_GET_PEER_REDIRECT,  // udp方式请求peer-list包的回复包
    BT_DHT_GET_PEER_REDIRECT,  // dht获取peer-list包的回复包
    BT_HANDSHAKE_REDIRECT,  // bt协议handshake包的回复包
    BT_PACKET_TYPE_END  // 所有的枚举在这之前定义（不要在此枚举之后定义任何枚举）
};

// peer以http get方式向tracker请求peer-list时的请求信息
struct BtGetPeerRequest
{
    std::string info_hash;  // 请求文件对应的info-hash
    EndPoint requester;  // 发送此请求的peer监听地址
    uint64 file_size = 0;  // peer需要下载的文件大小
    uint16 num_want = 0;  // peer向tracker请求其它peer的数目
    bool compact = false;
    std::string tracker;  // 发送此请求的目标tracker服务器
    std::string event;  // 发送此请求的peer当前文件下载状态
    time_t time = 0;  // peer发送此请求的时间
};

class BtSession;

typedef boost::shared_ptr<BtSession> BtSessionSP;

}  // BroadCache

#endif  // HEADER_BT_TYPES
