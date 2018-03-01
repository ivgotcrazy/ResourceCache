/*#############################################################################
 * �ļ���   : bt_msg_recognizer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��19��
 * �ļ����� : BtMsgRecognizer����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BT_MSG_RECOGNIZER
#define HEADER_BT_MSG_RECOGNIZER

#include "msg_recognizer.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: BT Handshake������Ϣʶ����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/19
 *****************************************************************************/
class BtHandshakeMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

/******************************************************************************
 * ����: BT Common������Ϣʶ����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/19
 *****************************************************************************/
class BtCommonMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

}

#endif