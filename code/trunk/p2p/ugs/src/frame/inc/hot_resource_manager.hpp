/*##############################################################################
 * �ļ���   : hot_resource_manager.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.12.05
 * �ļ����� : ��HotResourceManager�������ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_HOT_ALGORITHM
#define HEADER_HOT_ALGORITHM

#include <functional>
#include <boost/thread.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include "ugs_typedef.hpp"
#include "ugs_util.hpp"
#include "access_record_database.hpp"

using namespace boost::multi_index;

namespace BroadCache
{

class HotValueCalculator;

/*******************************************************************************
 * ��  ��: �����ж�ĳ����Դ�Ƿ����ȵ���Դ�������ʵ���ʱ��ɾ��û�м�ֵ����Դ
 * ��  ��: rosan
 * ʱ  ��: 2013.12.05
 ******************************************************************************/
class HotResourceManager : public boost::noncopyable
{
public:
    HotResourceManager();

	bool Init();

    // �ж�ĳ����Դ�Ƿ����ȵ���Դ
    bool IsHotResource(const AccessRecord& record);

private:
    struct Entry
    {
        ResourceId resource;  // ��Դ��ʶ
        hot_value_t value = 0.0;  // ��Դ�ļ�ֵ
    };

    struct Resource {};  // ��Դ��ͼ��ǩ
    struct Value {};  // ��ֵ��ͼ��ǩ

    struct EntryModifier  // �޸��������ݵĺ�������
    {
    public:
        explicit EntryModifier(hot_value_t value) : value_(value) {}

        void operator()(Entry& entry) const
        {
            entry.value = value_;
        }

    private:
        hot_value_t value_;
    };

private:
    // ����һ�����ʼ�¼
    void AddAccessRecord(const AccessRecord& record);

    // ����һ�����ʼ�¼
    void UpdateAccessRecord(const AccessRecord& new_record, const AccessRecord& old_record);

    // ɾ��û�м�ֵ��Դ���߳�
    void RemoveResource();

    // �޸���Դ��ֵ
    void ModifyResourceValue(const ResourceId& resource, hot_value_t value);

    // ��Դ��ֵ�����ı�Ļص�����
    void OnResourceValueChanged(const ResourceId& resource,
                                hot_value_t new_value,
                                hot_value_t old_value);

private:
    uint32 remove_resource_interval_ = 100;  // ɾ��û�м�ֵ��Դ��ʱ����

    mutable boost::mutex mutex_;
    boost::shared_ptr<HotValueCalculator> calculator_;
    boost::shared_ptr<boost::thread> remove_resource_thread_;  // ɾ��û�м�ֵ��Դ���߳�

	AccessRecordDatabase access_records_; // ���ʼ�¼����

    multi_index_container<
        Entry,
        indexed_by<
            hashed_unique<tag<Resource>, member<Entry, ResourceId, &Entry::resource> >,
            ordered_non_unique<tag<Value>, member<Entry, hot_value_t, &Entry::value>, std::greater<hot_value_t> >
        >
    > hot_values_;  // ������Դ��ֵ������
};

}  // namespace BroadCache

#endif  // HEADER_HOT_ALGORITHM
