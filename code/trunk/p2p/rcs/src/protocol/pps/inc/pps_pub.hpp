/*#############################################################################
 * 文件名   : pps_pub.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2014年1月3日
 * 文件描述 : BT协议内部公用宏、函数、常量等定义
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef PPS_PUB_HEADER
#define PPS_PUB_HEADER

#include "bc_typedef.hpp"

namespace BroadCache
{

#define PPS_FAST_RESUME_VERSION	"0.1.0"			// fast-resume版本
#define PPS_FAST_RESUME_DESC	"fast-resume"	// fast-resume描述
#define PPS_FAST_RESUME_PROTOCOL "pps"			// fast-resume协议类型

#define PPS_INFO_HASH_LEN	20

const uint8 kProtoPps = 0x43;

enum PpsMsgType
{
	PPS_MSG_BEGIN = 0,
	PPS_MSG_CHOKE = PPS_MSG_BEGIN,
	PPS_MSG_INFO_REQ = 0x0000ED00,
	PPS_MSG_INFO_RES = 0x0000EE00,
	PPS_MSG_HANDSHAKE = 0x00008200,
	PPS_MSG_REQUEST = 0x0000D800,
	PPS_MSG_PIECE = 0x0000D900,
	PPS_MSG_HEARTBEAT = 0x0000EC00,
	PPS_MSG_BITFIELD = 0x0000D200,
	PPS_MSG_EXTEND_REQUEST = 0x0000A100,
	PPS_MSG_EXTEND_PIECE = 0x0000A200,
	PPS_MSG_KEEPALIVE_REQ = 0x0000E300,
	PPS_MSG_KEEPALIVE_RES = 0x0000E700,
	PPS_MSG_PEERLIST_REQ = 0x4A71FF00,
	PPS_MSG_PEERLIST_RES = 0x4B17FF00,
	PPS_MSG_INFO_TRACKER_REQ = 0x4871FF00,
	PPS_MSG_INFO_TRACKER_RES = 0x4917FF00,

	PPS_MSG_UNKNOWN
};



uint64 CalcInnerChecksum(uint64 a1, unsigned char a2);
uint16 CalcChecksum(unsigned char * src, uint32 len, unsigned char a3);

uint32 CalcPpsPieceSize(uint32 file_size, uint32 piece_count);

}

#endif

