/*#############################################################################
 * 文件名   : bt_msg_recognizer.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月19日
 * 文件描述 : BtMsgRecognizer实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bt_msg_recognizer.hpp"
#include "bc_util.hpp"
#include "bt_pub.hpp"
#include "bc_io.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: BT握手消息识别
 * 参  数: [in] buf 消息识别器
 *         [in] len 消息处理函数
 *         [out] recognized_len 识别出的消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtHandshakeMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// BT握手消息格式如下：
	// ====================================================================
	//    1Byte          19Bytes         8Bytes     20Bytes      20Bytes
    // |--------|---------------------|----------|-----------------------|
    // |   19   | BitTorrent Protocol | reserved | info_hash  | peer_id  |
    // |--------|---------------------|----------|-----------------------|
	// <pstrlen><pstr><reserved><info_hash><peer_id>

	if (len < BT_HANDSHAKE_PKT_LEN) return false;

	if (*buf != BT_HANDSHAKE_PROTOCOL_STR_LEN) return false;

	if (std::strncmp(BT_HANDSHAKE_PROTOCOL_STR, buf + 1, BT_HANDSHAKE_PROTOCOL_STR_LEN) != 0)
	{
		return false;
	}

	*recognized_len = BT_HANDSHAKE_PKT_LEN;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: BT普通消息识别
 * 参  数: [in] buf 消息识别器
 *         [in] len 消息处理函数
 *         [out] recognized_len 识别出的消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtCommonMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// BT普通消息格式如下：
	// ============================================
	//       4Bytes         1Byte    
    // |---------------|------------|--------------
    // | length prefix | message ID | payload ...
    // |---------------|------------|--------------
	// <length prefix><message ID><payload>

	uint32 length_prefix = IO::read_uint32(buf);

	if (length_prefix + 4 > len) return false;

	*recognized_len = length_prefix + 4;

	return true;
}

}
