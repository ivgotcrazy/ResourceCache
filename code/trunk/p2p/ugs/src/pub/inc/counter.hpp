/*##############################################################################
 * 文件名   : counter.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.19
 * 文件描述 : 实现多线程安全类Counter 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_COUNTER
#define HEADER_COUNTER

#include <boost/thread.hpp>

#include "bc_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 *描  述: 此类实现计数功能
 *作  者: rosan
 *时  间: 2013.11.19
 ******************************************************************************/
class Counter
{
public:
    typedef uint32 counter_t;

public:
    inline explicit Counter(counter_t init_value = counter_t());

    // 获取计数
    inline counter_t Get() const;

    // 递增计数
    inline void Increase();

    // 清空计数
    inline void Clear();

private:
    counter_t counter_;  // 计数
    mutable boost::shared_mutex mutex_;  // 读写锁
};

}  // namespace BroadCache

#include "counter_inl.hpp"

#endif  // HEADER_COUNTER
