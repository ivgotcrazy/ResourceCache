/*##############################################################################
 * 文件名   : hot_algorithm.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.12.05
 * 文件描述 : 类HotResourceManager的实现文件
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "hot_resource_manager.hpp"
#include "hot_value_calculator.hpp"
#include "access_record_database.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: HotResourceManager构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
HotResourceManager::HotResourceManager()
{
    calculator_.reset(new HotValueCalculator());
    calculator_->SetHotValueChangeHandler(
        boost::bind(&HotResourceManager::OnResourceValueChanged, this, _1, _2, _3));

    remove_resource_thread_.reset(
        new boost::thread(boost::bind(&HotResourceManager::RemoveResource, this)));
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年12月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool HotResourceManager::Init()
{
	if (!access_records_.Init())
	{
		LOG(ERROR) << "Fail to init access record database";
		return false;
	}

	access_records_.SetRecordAddedHandler(
		boost::bind(&HotResourceManager::AddAccessRecord, this, _1));

	access_records_.SetRecordUpdatedHandler(
		boost::bind(&HotResourceManager::UpdateAccessRecord, this, _1, _2));

	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 判断某个资源是否是热点资源
 * 参  数: [in] resource 资源标识
 * 返回值: 资源是否是热点资源
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool HotResourceManager::IsHotResource(const AccessRecord& record)
{
    static const size_t kMax = 20;

    access_records_.AddAccessRecord(record);

    boost::lock_guard<decltype(mutex_)> lock(mutex_);

    auto& resource_view = hot_values_.get<Resource>();
    auto i = resource_view.find(record.resource);

    if (i == resource_view.end())  // 没有找到此资源
        return false;

    if (resource_view.size() < kMax)
        return true;

    auto j = hot_values_.get<Value>().begin();
    std::advance(j, kMax - 1);

    return i->value >= j->value;
}

/*------------------------------------------------------------------------------
 * 描  述: 修改资源的价值
 * 参  数: [in] resource 资源标识
 *         [in] value 资源的价值
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::ModifyResourceValue(const ResourceId& resource,
                                       hot_value_t value)
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);

    auto& resource_view = hot_values_.get<Resource>();
    auto i = resource_view.find(resource);
    if (i == resource_view.end())  // 未找到此资源
    {
        Entry entry;
        entry.resource = resource;
        entry.value = value;
        resource_view.insert(entry);
    }
    else
    {
        resource_view.modify(i, EntryModifier(value));
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 当资源价值发生改变时的回调函数
 * 参  数: [in] source 资源标识
 *         [in] new_value 资源新的价值
 *         [in] old_value 资源旧的价值
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::OnResourceValueChanged(const ResourceId& resource,
                                hot_value_t new_value,
                                hot_value_t /*old_value*/)
{
    ModifyResourceValue(resource, new_value);
}

/*------------------------------------------------------------------------------
 * 描  述: 当资源访问记录更新时的回调函数
 * 参  数: [in] new_record 新的访问记录
 *         [in] old_record 旧的访问记录
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::UpdateAccessRecord(const AccessRecord& new_record,
                                      const AccessRecord& old_record)
{
    calculator_->UpdateAccessRecord(new_record, old_record);
}

/*------------------------------------------------------------------------------
 * 描  述: 当资源被访问时的回调函数
 * 参  数: [in] record 资源访问记录
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::AddAccessRecord(const AccessRecord& record)
{
    calculator_->AddAccessRecord(record);
}

/*------------------------------------------------------------------------------
 * 描  述: 删除没有价值的资源的线程
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::RemoveResource()
{
    // static const size_t kMaxEntrySize = 1024 * 1024;  // 资源的最大数目
    static const size_t kMaxEntrySize = 1024 * 16;  // 资源的最大数目

    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(remove_resource_interval_));

        std::vector<boost::function<void()> > callbacks;

        mutex_.lock();

        auto& value_view = hot_values_.get<Value>();
        while (value_view.size() > kMaxEntrySize)
        {
            auto i = --value_view.end();

            callbacks.push_back(boost::bind(
                &HotValueCalculator::RemoveResource, calculator_, i->resource));
            callbacks.push_back(boost::bind(
                &AccessRecordDatabase::RemoveResource, &access_records_, i->resource));

            value_view.erase(i);
        }
        
        mutex_.unlock();

        FOREACH(callback, callbacks)
        {
            callback();
        }
    }
}

}  // namespace BroadCache
