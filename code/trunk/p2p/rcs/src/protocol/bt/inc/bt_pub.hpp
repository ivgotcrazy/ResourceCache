/*#############################################################################
 * 文件名   : bt_pub.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月20日
 * 文件描述 : BT协议内部公用宏、函数、常量等定义
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BT_PUB
#define HEADER_BT_PUB

namespace BroadCache
{

#define BT_FAST_RESUME_VERSION	"0.1.0"			// fast-resume版本
#define BT_FAST_RESUME_DESC		"fast-resume"	// fast-resume描述
#define BT_FAST_RESUME_PROTOCOL "bt"			// fast-resume协议类型

#define BT_INFO_HASH_LEN	20

// BT握手消息格式如下：
// ====================================================================
//    1Byte          19Bytes         8Bytes     20Bytes      20Bytes
// |--------|---------------------|----------|-----------------------|
// |   19   | BitTorrent Protocol | reserved | info_hash  | peer_id  |
// |--------|---------------------|----------|-----------------------|
// <pstrlen><pstr><reserved><info_hash><peer_id>
#define BT_HANDSHAKE_PROTOCOL_STR		"BitTorrent protocol"
#define BT_HANDSHAKE_PROTOCOL_STR_LEN	19
#define BT_HANDSHAKE_PKT_LEN			(1 + 19 + 8 + 20 + 20)

}

#endif