/*#############################################################################
 * �ļ���   : msg_recognizer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��19��
 * �ļ����� : MsgRecognizer����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_MSG_RECOGNIZER
#define HEADER_MSG_RECOGNIZER

#include <boost/noncopyable.hpp>

#include "bc_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: Messageʶ����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/19
 *****************************************************************************/
class MsgRecognizer : public boost::noncopyable
{
public:
	virtual ~MsgRecognizer() {}

	// ��buf�ĵ�һ���ֽڿ�ʼƥ�䣬��С�ڵ���len�ĳ��ȷ�Χ�ڣ�ʶ���һ����Ϣ��
	// [in] buf: ��������
	// [in] len: ���ĳ���
	// [out] recognized_len: ʶ�������Ϣ�ĳ���
	// ����true����ʾʶ�����Ϣ������false����ʾδʶ�����Ϣ��
	virtual bool RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len) = 0;
};

}

#endif