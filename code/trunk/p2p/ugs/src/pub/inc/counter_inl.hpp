/*##############################################################################
 * 文件名   : counter_inl.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.19
 * 文件描述 : Counter类的实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_COUNTER_INL
#define HEADER_COUNTER_INL

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: Counter类的构造函数
 * 参  数: [in] init_value 计数的初始值
 * 返回值:
 * 修  改:
 *   时间 2013.11.19
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline Counter::Counter(counter_t init_value) : counter_(init_value)
{
}

/*------------------------------------------------------------------------------
 * 描  述: 获取计数
 * 参  数:
 * 返回值: 计数
 * 修  改:
 *   时间 2013.11.19
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline Counter::counter_t Counter::Get() const
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);
    return counter_;
}

/*------------------------------------------------------------------------------
 * 描  述: 递增计数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.19
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline void Counter::Increase()
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);
    ++counter_;
}

/*------------------------------------------------------------------------------
 * 描  述: 清空计数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.19
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline void Counter::Clear()
{
    boost::lock_guard<decltype(mutex_)> guard(mutex_);
    counter_ = 0;
}

}  // BroadCache

#endif  // HEADER_COUNTER_INL
