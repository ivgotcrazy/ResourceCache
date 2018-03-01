/*##############################################################################
 * 文件名   : hot_value_calculator.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.12.05
 * 文件描述 : 类HotValueCalculator实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "hot_value_calculator.hpp"
#include <ctime>
#include <limits>

#include "ugs_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: HotValueCalculator类的构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
HotValueCalculator::HotValueCalculator()
{
    calculate_thread_.reset(new boost::thread(
        boost::bind(&HotValueCalculator::Calculate, this)));
}

/*------------------------------------------------------------------------------
 * 描  述: 获取当资源的值发生改变时的回调函数
 * 参  数:
 * 返回值: 资源值发生改变时的回调函数
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
HotValueCalculator::HotValueChangeHandler HotValueCalculator::GetHotValueChangeHandler() const
{
    return hot_value_change_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置资源值发生改变时的回调函数
 * 参  数: [in] handler 资源值发生改变时的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotValueCalculator::SetHotValueChangeHandler(const HotValueChangeHandler& handler)
{
    hot_value_change_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取资源的价值
 * 参  数: [in] resource 资源标识
 * 返回值: 资源的价值
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
hot_value_t HotValueCalculator::GetHotValue(const ResourceId& resource) const
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);

    auto i = hot_values_.find(resource);
    return (i != hot_values_.end()) ? i->second.value : hot_value_t();
}   

/*------------------------------------------------------------------------------
 * 描  述: 增加一条资源访问记录
 * 参  数: [in] record 资源访问记录
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotValueCalculator::AddAccessRecord(const AccessRecord& record)
{
    mutex_.lock();

    auto& entry = hot_values_[record.resource];
    hot_value_t old_value = entry.value;

    ++entry.access_times;
    entry.sum += record.access_time;
    entry.quadratic_sum += record.access_time * record.access_time;
    hot_value_t new_value = CalculateHotValue(entry.sum, entry.quadratic_sum,
                                         entry.access_times, time(nullptr));
    entry.value = new_value;

    mutex_.unlock();

    if (hot_value_change_handler_)
    {
        hot_value_change_handler_(record.resource, new_value, old_value);
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 计算资源的价值
 * 参  数: [in] sum 资源访问时间相加的和
 *         [in] quadratic_sum 资源访问时间的平方相加的和
 *         [in] n 资源访问的次数
 *         [in] curr_time 当前时间
 * 返回值: 资源的价值
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
hot_value_t HotValueCalculator::CalculateHotValue(hot_value_t sum,
                                                  hot_value_t quadratic_sum,
                                                  access_times_t n,
                                                  time_t curr_time)
{
    hot_value_t mu = sum / n;  // 资源访问时间均值
    hot_value_t sigma_quadratic = quadratic_sum / n - mu * mu;  // 资源访问时间的方差

    if (sigma_quadratic < std::numeric_limits<hot_value_t>::epsilon())  // 方差为0
        return hot_value_t();

    auto x = hot_value_t(curr_time);
    auto dx = x - mu;

    // return n * pow(M_E, -dx * dx / (100000.0 * sigma_quadratic));
    // return n * pow(M_E, -dx * dx / (100.0 * sigma_quadratic));
    return n * pow(M_E, -dx * dx / (2.0 * sigma_quadratic));
}

/*------------------------------------------------------------------------------
 * 描  述: 更新一条访问记录
 * 参  数: [in] new_record 新的访问记录
 *         [in] old_record 旧的访问记录（这个记录会被新的访问记录所替换）
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotValueCalculator::UpdateAccessRecord(const AccessRecord& new_record,
                                            const AccessRecord& old_record)
{
    if (new_record.resource != old_record.resource)
        return ;

    mutex_.lock();
    
    auto i = hot_values_.find(old_record.resource);
    if (i == hot_values_.end())
        return ;

    auto& entry = i->second;
    hot_value_t old_value = entry.value;

    entry.sum -= old_record.access_time;
    entry.sum += new_record.access_time;
    entry.quadratic_sum -= old_record.access_time * old_record.access_time;
    entry.quadratic_sum += new_record.access_time * new_record.access_time;
    hot_value_t new_value = CalculateHotValue(entry.sum, entry.quadratic_sum,
                                    entry.access_times, time(nullptr));
    entry.value = new_value; 

    mutex_.unlock();

    if (hot_value_change_handler_)
    {
        hot_value_change_handler_(new_record.resource, new_value, old_value);
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 删除资源
 * 参  数: [in] resource 资源标识
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotValueCalculator::RemoveResource(const ResourceId& resource)
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);
    hot_values_.erase(resource);
}

/*------------------------------------------------------------------------------
 * 描  述: 实时更新资源价值的线程
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotValueCalculator::Calculate()
{
    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::seconds(calculate_interval_));

        std::vector<boost::function<void()> > callbacks;
        time_t curr_time = time(nullptr);

        mutex_.lock();
        FOREACH(e, hot_values_)
        {
            Entry& entry = e.second;
            hot_value_t old_value = entry.value;
            entry.value = CalculateHotValue(entry.sum, entry.quadratic_sum,
                    entry.access_times, curr_time);

            LOG(INFO) << entry.value;
            callbacks.push_back(boost::bind(
                hot_value_change_handler_, e.first, entry.value, old_value));
        }
        mutex_.unlock();

        FOREACH(callback, callbacks)
        {
            callback();
        }
    }
}

}  // namespace BroadCache
