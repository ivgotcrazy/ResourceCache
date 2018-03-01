/*##############################################################################
 * �ļ���   : counter.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.19
 * �ļ����� : ʵ�ֶ��̰߳�ȫ��Counter 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_COUNTER
#define HEADER_COUNTER

#include <boost/thread.hpp>

#include "bc_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 *��  ��: ����ʵ�ּ�������
 *��  ��: rosan
 *ʱ  ��: 2013.11.19
 ******************************************************************************/
class Counter
{
public:
    typedef uint32 counter_t;

public:
    inline explicit Counter(counter_t init_value = counter_t());

    // ��ȡ����
    inline counter_t Get() const;

    // ��������
    inline void Increase();

    // ��ռ���
    inline void Clear();

private:
    counter_t counter_;  // ����
    mutable boost::shared_mutex mutex_;  // ��д��
};

}  // namespace BroadCache

#include "counter_inl.hpp"

#endif  // HEADER_COUNTER
