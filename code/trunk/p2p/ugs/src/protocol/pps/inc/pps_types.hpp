/*##############################################################################
 * 文件名   : pps_types.hpp
 * 创建人   : tom_liu 
 * 创建时间 : 2013.12.24
 * 文件描述 : 此文件定义了pps协议模块的相关通用结构和类型
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_PPS_TYPES
#define HEADER_PPS_TYPES

#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

static const uint32 kPpsGetPeerRequestMax = 1024;  // 保存的http get peer请求的最大个数

// bt数据包类型
enum PpsPacketType
{
    PPS_PACKET_TYPE_BEGIN = 0,  // 不要在此枚举之前定义任何枚举
    PPS_COMMON_PACKET = PPS_PACKET_TYPE_BEGIN,
    PPS_GET_PEER,  // 请求peer-list包
    PPS_HANDSHAKE,  // pps协议的handshake包
    PPS_GET_META_INFO, //请求meta信息
    PPS_GET_BASE_INFO, //请求base信息
    PPS_PACKET_TYPE_END  // 所有的枚举在这之前定义（不要在此枚举之后定义任何枚举）
};

// pps数据包类型对应的字符串信息
const char* const kPpsPktTypeDescriptions[PPS_PACKET_TYPE_END] = 
{
	"common",
	"Get peer",
	"Pps handshake",
	"Get meta info",
	"Get base info"
};

class PpsSession;

typedef boost::shared_ptr<PpsSession> PpsSessionSP;

}  // BroadCache

#endif  // HEADER_BT_TYPES
