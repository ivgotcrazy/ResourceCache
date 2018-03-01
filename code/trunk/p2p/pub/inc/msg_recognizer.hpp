/*#############################################################################
 * 文件名   : msg_recognizer.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月19日
 * 文件描述 : MsgRecognizer声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_MSG_RECOGNIZER
#define HEADER_MSG_RECOGNIZER

#include <boost/noncopyable.hpp>

#include "bc_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: Message识别器
 * 作者：teck_zhou
 * 时间：2013/11/19
 *****************************************************************************/
class MsgRecognizer : public boost::noncopyable
{
public:
	virtual ~MsgRecognizer() {}

	// 从buf的第一个字节开始匹配，在小于等于len的长度范围内，识别出一个消息。
	// [in] buf: 报文数据
	// [in] len: 报文长度
	// [out] recognized_len: 识别出的消息的长度
	// 返回true，表示识别出消息；返回false，表示未识别出消息。
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) = 0;
};

}

#endif