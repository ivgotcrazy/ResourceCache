/*#############################################################################
 * 文件名   : bt_msg_recognizer.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月19日
 * 文件描述 : BtMsgRecognizer声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BT_MSG_RECOGNIZER
#define HEADER_BT_MSG_RECOGNIZER

#include "msg_recognizer.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: BT Handshake报文消息识别器
 * 作者：teck_zhou
 * 时间：2013/11/19
 *****************************************************************************/
class BtHandshakeMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

/******************************************************************************
 * 描述: BT Common报文消息识别器
 * 作者：teck_zhou
 * 时间：2013/11/19
 *****************************************************************************/
class BtCommonMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

}

#endif