/*##############################################################################
 * �ļ���   : hot_value_calculator.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.12.05
 * �ļ����� : ��HotValueCalculator�Ĺ��캯�� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_HOT_VALUE_CALCULATOR
#define HEADER_HOT_VALUE_CALCULATOR

#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>

#include "ugs_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 * ��  ��: ��������ʵʱ���������Դ�ȶ�
 * ��  ��: rosan
 * ʱ  ��: 2013.12.05
 ******************************************************************************/
class HotValueCalculator
{
public:
    typedef boost::function<void(const ResourceId& resource, hot_value_t new_value,
        hot_value_t old_value)> HotValueChangeHandler;  // ��Դ�ȶȸı��Ļص�����

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
        access_times_t access_times = 0; // ��Դ���ʴ���
        double sum = 0.0;				 // ��Դ����ʱ���ܺ� 
        double quadratic_sum = 0.0;	     // ��Դ����ʱ��ƽ����
        hot_value_t value = 0.0;		 // ��Դ��ֵ
    };

private:
    // ʵʱ������Դ��ֵ���߳�
    void Calculate();

    // ������Դ�ȶȵĺ���
    static hot_value_t CalculateHotValue(hot_value_t sum,
        hot_value_t quadratic_sum, access_times_t n, time_t curr_time);

private:
    uint32 calculate_interval_ = 3;  // ������Դ�ȶȵļ��ʱ�䣨��λ���룩

    mutable boost::shared_mutex mutex_;
    boost::shared_ptr<boost::thread> calculate_thread_;  // ������Դ�ȶȵ��߳�
    HotValueChangeHandler hot_value_change_handler_;	 // ��Դ�ȶȸ��º�Ļص�����
    boost::unordered_map<ResourceId, Entry> hot_values_; // ������Դ�ȶȵ�����
};

}  // namespace BroadCache

#endif  // HEADER_HOT_VALUE_CALCULATOR
