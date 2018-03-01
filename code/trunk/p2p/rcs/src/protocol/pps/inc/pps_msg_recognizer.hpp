/*#############################################################################
 * �ļ���   : pps_msg_recognizer.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��30��
 * �ļ����� : PpsMsgRecognizer����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PPS_MSG_RECOGNIZER
#define HEADER_PPS_MSG_RECOGNIZER

#include "msg_recognizer.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: PPS������Ϣʶ����
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
 *****************************************************************************/
class PpsCommonMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

}
#endif

