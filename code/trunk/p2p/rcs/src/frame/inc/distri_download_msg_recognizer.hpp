/*#############################################################################
 * 文件名   : distri_download_msg_recognizer.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月19日
 * 文件描述 : DistriDownloadMsgRecognizer声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/


#ifndef HEADER_DISTRI_DOWNLOAD_MSG_RECOGNIZER
#define HEADER_DISTRI_DOWNLOAD_MSG_RECOGNIZER

#include "msg_recognizer.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: 分布式下载协议报文消息识别器
 * 作者：teck_zhou
 * 时间：2013/11/19
 *****************************************************************************/
class DistriDownloadMsgRecognizer : public MsgRecognizer
{
public:
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) override;
};

}

#endif
