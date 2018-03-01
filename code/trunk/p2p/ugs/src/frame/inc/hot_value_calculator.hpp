/*##############################################################################
 * 文件名   : hot_value_calculator.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.12.05
 * 文件描述 : 类HotValueCalculator的构造函数 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_HOT_VALUE_CALCULATOR
#define HEADER_HOT_VALUE_CALCULATOR

#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>

#include "ugs_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 * 描  述: 此类用于实时计算更新资源热度
 * 作  者: rosan
 * 时  间: 2013.12.05
 ******************************************************************************/
class HotValueCalculator
{
public:
    typedef boost::function<void(const ResourceId& resource, hot_value_t new_value,
        hot_value_t old_value)> HotValueChangeHandler;  // 资源热度改变后的回调函数

public:
    HotValueCalculator();

    hot_value_t GetHotValue(const ResourceId& resource) const;
    void RemoveResource(const ResourceId& resource);
    
    void AddAccessRecord(const AccessRecord& record);
    void UpdateAccessRecord(const AccessRecord& new_record, const AccessRecord& old_record);

    HotValueChangeHandler GetHotValueChangeHandler() const;
    void SetHotValueChangeHandler(const HotValueChangeHandler& handler);

private:
    struct Entry
    {
        access_times_t access_times = 0; // 资源访问次数
        double sum = 0.0;				 // 资源访问时间总和 
        double quadratic_sum = 0.0;	     // 资源访问时间平方和
        hot_value_t value = 0.0;		 // 资源价值
    };

private:
    // 实时更新资源价值的线程
    void Calculate();

    // 计算资源热度的函数
    static hot_value_t CalculateHotValue(hot_value_t sum,
        hot_value_t quadratic_sum, access_times_t n, time_t curr_time);

private:
    uint32 calculate_interval_ = 3;  // 更新资源热度的间隔时间（单位：秒）

    mutable boost::shared_mutex mutex_;
    boost::shared_ptr<boost::thread> calculate_thread_;  // 更新资源热度的线程
    HotValueChangeHandler hot_value_change_handler_;	 // 资源热度更新后的回调函数
    boost::unordered_map<ResourceId, Entry> hot_values_; // 保存资源热度的容器
};

}  // namespace BroadCache

#endif  // HEADER_HOT_VALUE_CALCULATOR
