/*#############################################################################
 * 文件名   : distri_download_msg_recognizer.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月19日
 * 文件描述 : DistriDownloadMsgRecognizer实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <glog/logging.h>

#include "distri_download_msg_recognizer.hpp"
#include "bc_util.hpp"
#include "bc_io.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 私有协议消息识别
 * 参  数: [in] buf 消息识别器
 *         [in] len 消息处理函数
 *         [out] recognized_len 识别出的消息长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool DistriDownloadMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// 私有报文格式如下：
	// ====================================================================
	//     			4Bytes                    2Bytes        1Byte
    // |--------------------------------|----------------|--------|--------
    // |         magic-number           |     length     |  type  | data...
    // |--------------------------------|----------------|--------|--------

	if (len < 4 + 2 + 1) return false;

	const char* tmp = buf;

	uint32 magic_number = IO::read_uint32(tmp);

	// 匹配魔术字
	if (DISTRIBUTED_DOWNLOAD_MAGIC_NUMBER != magic_number) return false;

	uint32 pkt_len = IO::read_uint16(tmp);
	
	// 校验长度字段，理论上这里应该要校验通过
	if (pkt_len + 4 + 2 > len) 
	{
		LOG(WARNING) << "Fail to check length for private msg | pkt_len: " 
			         << pkt_len << " len: " << len;
		return false;
	}

	*recognized_len = pkt_len + 4 + 2;

	return true;
}

}