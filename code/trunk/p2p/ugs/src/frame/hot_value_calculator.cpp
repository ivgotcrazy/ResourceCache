/*##############################################################################
 * �ļ���   : hot_value_calculator.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.12.05
 * �ļ����� : ��HotValueCalculatorʵ���ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "hot_value_calculator.hpp"
#include <ctime>
#include <limits>

#include "ugs_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: HotValueCalculator��Ĺ��캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
HotValueCalculator::HotValueCalculator()
{
    calculate_thread_.reset(new boost::thread(
        boost::bind(&HotValueCalculator::Calculate, this)));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ����Դ��ֵ�����ı�ʱ�Ļص�����
 * ��  ��:
 * ����ֵ: ��Դֵ�����ı�ʱ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
HotValueCalculator::HotValueChangeHandler HotValueCalculator::GetHotValueChangeHandler() const
{
    return hot_value_change_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ������Դֵ�����ı�ʱ�Ļص�����
 * ��  ��: [in] handler ��Դֵ�����ı�ʱ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void HotValueCalculator::SetHotValueChangeHandler(const HotValueChangeHandler& handler)
{
    hot_value_change_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ��Դ�ļ�ֵ
 * ��  ��: [in] resource ��Դ��ʶ
 * ����ֵ: ��Դ�ļ�ֵ
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
hot_value_t HotValueCalculator::GetHotValue(const ResourceId& resource) const
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);

    auto i = hot_values_.find(resource);
    return (i != hot_values_.end()) ? i->second.value : hot_value_t();
}   

/*------------------------------------------------------------------------------
 * ��  ��: ����һ����Դ���ʼ�¼
 * ��  ��: [in] record ��Դ���ʼ�¼
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ������Դ�ļ�ֵ
 * ��  ��: [in] sum ��Դ����ʱ����ӵĺ�
 *         [in] quadratic_sum ��Դ����ʱ���ƽ����ӵĺ�
 *         [in] n ��Դ���ʵĴ���
 *         [in] curr_time ��ǰʱ��
 * ����ֵ: ��Դ�ļ�ֵ
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
hot_value_t HotValueCalculator::CalculateHotValue(hot_value_t sum,
                                                  hot_value_t quadratic_sum,
                                                  access_times_t n,
                                                  time_t curr_time)
{
    hot_value_t mu = sum / n;  // ��Դ����ʱ���ֵ
    hot_value_t sigma_quadratic = quadratic_sum / n - mu * mu;  // ��Դ����ʱ��ķ���

    if (sigma_quadratic < std::numeric_limits<hot_value_t>::epsilon())  // ����Ϊ0
        return hot_value_t();

    auto x = hot_value_t(curr_time);
    auto dx = x - mu;

    // return n * pow(M_E, -dx * dx / (100000.0 * sigma_quadratic));
    // return n * pow(M_E, -dx * dx / (100.0 * sigma_quadratic));
    return n * pow(M_E, -dx * dx / (2.0 * sigma_quadratic));
}

/*------------------------------------------------------------------------------
 * ��  ��: ����һ�����ʼ�¼
 * ��  ��: [in] new_record �µķ��ʼ�¼
 *         [in] old_record �ɵķ��ʼ�¼�������¼�ᱻ�µķ��ʼ�¼���滻��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ɾ����Դ
 * ��  ��: [in] resource ��Դ��ʶ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void HotValueCalculator::RemoveResource(const ResourceId& resource)
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);
    hot_values_.erase(resource);
}

/*------------------------------------------------------------------------------
 * ��  ��: ʵʱ������Դ��ֵ���߳�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
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
