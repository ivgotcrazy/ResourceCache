/*#############################################################################
 * �ļ���   : distri_download_msg_recognizer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��19��
 * �ļ����� : DistriDownloadMsgRecognizer����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/


#ifndef HEADER_DISTRI_DOWNLOAD_MSG_RECOGNIZER
#define HEADER_DISTRI_DOWNLOAD_MSG_RECOGNIZER

#include "msg_recognizer.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: �ֲ�ʽ����Э�鱨����Ϣʶ����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/19
 *****************************************************************************/
class DistriDownloadMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

}

#endif
