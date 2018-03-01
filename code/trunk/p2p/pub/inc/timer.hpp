/*#############################################################################
 * 文件名   : timer.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月10日
 * 文件描述 : Timer声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_TIMER
#define HEADER_TIMER

#include "bc_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: 对boost定时器的封装
 * 作者：teck_zhou
 * 时间：2013/11/10
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