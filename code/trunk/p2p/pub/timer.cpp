/*#############################################################################
 * 文件名   : timer.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月10日
 * 文件描述 : Timer实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "timer.hpp"

#include "depend.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: [in] ios ios_service
 *         [in] callback 回调函数
 *         [in] interval 定时器时长
 * 返回值:
 * 修  改:
 *   时间 2013年11月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Timer::Timer(io_service& ios, TimerCallback callback, uint32 interval)
	: timer_(ios), callback_(callback) , interval_(interval)
{
	BC_ASSERT(callback_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器回调函数
 * 参  数: [in] ec 错误码
 * 返回值:
 * 修  改:
 *   时间 2013年11月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Timer::OnTick(const error_code& ec)
{
	if (ec) return;

	if (callback_) callback_();

	timer_.expires_from_now(boost::posix_time::seconds(interval_));
	timer_.async_wait(boost::bind(&Timer::OnTick, this, _1));
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动定时器
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Timer::Start()
{
	timer_.expires_from_now(boost::posix_time::seconds(interval_));
	timer_.async_wait(boost::bind(&Timer::OnTick, this, _1));
}

}