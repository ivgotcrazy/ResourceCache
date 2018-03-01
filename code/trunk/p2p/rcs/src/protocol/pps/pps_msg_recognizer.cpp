/*#############################################################################
 * 文件名   : pps_msg_recognizer.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月30日
 * 文件描述 : BtMsgRecognizer实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_msg_recognizer.hpp"
#include "bc_util.hpp"
#include "bc_io.hpp"
#include "pps_pub.hpp"
#include "net_byte_order.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: Pps消息识别
 * 参  数: [in] buf 消息识别器
 *         [in] len 消息处理函数
 *         [out] recognized_len 识别出的消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年12月30日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsCommonMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// Pps普通消息格式如下：
	// ============================================
	//       2Bytes         5Bytes    
    // |---------------|------------|--------------
    // | length prefix | message ID | payload ...
    // |---------------|------------|--------------
	// <length prefix><message ID><payload>

	uint16 length_prefix = IO::read_int16_be(buf);
						   
	LOG(INFO) << "length_prefix  " <<length_prefix << "len  " << (uint16)len;

	if (length_prefix > (uint16)len) return false;

	buf += 1;
	PpsMsgType type = static_cast<PpsMsgType>(IO::read_uint32(buf));

	LOG(INFO) << " Show message | type: " << type;

	if (type != PPS_MSG_HANDSHAKE || length_prefix == len) //handshake发送报文报长减4,回应报文不用减4， 暂不对这个进行处理
	{
		*recognized_len = length_prefix;
	}
	else
	{
		*recognized_len = length_prefix + 4;
	}

	return true;
}

}

