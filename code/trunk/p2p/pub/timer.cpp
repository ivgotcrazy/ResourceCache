/*#############################################################################
 * �ļ���   : timer.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��10��
 * �ļ����� : Timerʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "timer.hpp"

#include "depend.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: [in] ios ios_service
 *         [in] callback �ص�����
 *         [in] interval ��ʱ��ʱ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
Timer::Timer(io_service& ios, TimerCallback callback, uint32 interval)
	: timer_(ios), callback_(callback) , interval_(interval)
{
	BC_ASSERT(callback_);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ���ص�����
 * ��  ��: [in] ec ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Timer::OnTick(const error_code& ec)
{
	if (ec) return;

	if (callback_) callback_();

	timer_.expires_from_now(boost::posix_time::seconds(interval_));
	timer_.async_wait(boost::bind(&Timer::OnTick, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ʱ��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Timer::Start()
{
	timer_.expires_from_now(boost::posix_time::seconds(interval_));
	timer_.async_wait(boost::bind(&Timer::OnTick, this, _1));
}

}