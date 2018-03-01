/*#############################################################################
 * 文件名   : pps_msg_recognizer.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月30日
 * 文件描述 : PpsMsgRecognizer声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PPS_MSG_RECOGNIZER
#define HEADER_PPS_MSG_RECOGNIZER

#include "msg_recognizer.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: PPS报文消息识别器
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsCommonMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

}
#endif

