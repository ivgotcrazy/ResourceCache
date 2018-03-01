/*##############################################################################
 * �ļ���   : hot_algorithm.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.12.05
 * �ļ����� : ��HotResourceManager��ʵ���ļ�
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "hot_resource_manager.hpp"
#include "hot_value_calculator.hpp"
#include "access_record_database.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: HotResourceManager���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ��ʼ��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��24��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �ж�ĳ����Դ�Ƿ����ȵ���Դ
 * ��  ��: [in] resource ��Դ��ʶ
 * ����ֵ: ��Դ�Ƿ����ȵ���Դ
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool HotResourceManager::IsHotResource(const AccessRecord& record)
{
    static const size_t kMax = 20;

    access_records_.AddAccessRecord(record);

    boost::lock_guard<decltype(mutex_)> lock(mutex_);

    auto& resource_view = hot_values_.get<Resource>();
    auto i = resource_view.find(record.resource);

    if (i == resource_view.end())  // û���ҵ�����Դ
        return false;

    if (resource_view.size() < kMax)
        return true;

    auto j = hot_values_.get<Value>().begin();
    std::advance(j, kMax - 1);

    return i->value >= j->value;
}

/*------------------------------------------------------------------------------
 * ��  ��: �޸���Դ�ļ�ֵ
 * ��  ��: [in] resource ��Դ��ʶ
 *         [in] value ��Դ�ļ�ֵ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::ModifyResourceValue(const ResourceId& resource,
                                       hot_value_t value)
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);

    auto& resource_view = hot_values_.get<Resource>();
    auto i = resource_view.find(resource);
    if (i == resource_view.end())  // δ�ҵ�����Դ
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
 * ��  ��: ����Դ��ֵ�����ı�ʱ�Ļص�����
 * ��  ��: [in] source ��Դ��ʶ
 *         [in] new_value ��Դ�µļ�ֵ
 *         [in] old_value ��Դ�ɵļ�ֵ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::OnResourceValueChanged(const ResourceId& resource,
                                hot_value_t new_value,
                                hot_value_t /*old_value*/)
{
    ModifyResourceValue(resource, new_value);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����Դ���ʼ�¼����ʱ�Ļص�����
 * ��  ��: [in] new_record �µķ��ʼ�¼
 *         [in] old_record �ɵķ��ʼ�¼
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::UpdateAccessRecord(const AccessRecord& new_record,
                                      const AccessRecord& old_record)
{
    calculator_->UpdateAccessRecord(new_record, old_record);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����Դ������ʱ�Ļص�����
 * ��  ��: [in] record ��Դ���ʼ�¼
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::AddAccessRecord(const AccessRecord& record)
{
    calculator_->AddAccessRecord(record);
}

/*------------------------------------------------------------------------------
 * ��  ��: ɾ��û�м�ֵ����Դ���߳�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void HotResourceManager::RemoveResource()
{
    // static const size_t kMaxEntrySize = 1024 * 1024;  // ��Դ�������Ŀ
    static const size_t kMaxEntrySize = 1024 * 16;  // ��Դ�������Ŀ

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
