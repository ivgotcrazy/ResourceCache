/*#############################################################################
 * �ļ���   : timer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��10��
 * �ļ����� : Timer����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_TIMER
#define HEADER_TIMER

#include "bc_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: ��boost��ʱ���ķ�װ
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/10
 *****************************************************************************/
class Timer : public boost::noncopyable
{
public:
	typedef boost::function<void()> TimerCallback;

	Timer(io_service& ios, TimerCallback callback, uint32 interval);

	void Start();

private:
	void OnTick(const error_code& ec);

private:
	deadline_timer timer_;
	TimerCallback callback_;
	uint32 interval_;
};

}

#endif