/*##############################################################################
 * 文件名   : hot_resource_manager.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.12.05
 * 文件描述 : 类HotResourceManager的声明文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
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
 * 描  述: 用于判断某个资源是否是热点资源，并在适当的时候删除没有价值的资源
 * 作  者: rosan
 * 时  间: 2013.12.05
 ******************************************************************************/
class HotResourceManager : public boost::noncopyable
{
public:
    HotResourceManager();

	bool Init();

    // 判断某个资源是否是热点资源
    bool IsHotResource(const AccessRecord& record);

private:
    struct Entry
    {
        ResourceId resource;  // 资源标识
        hot_value_t value = 0.0;  // 资源的价值
    };

    struct Resource {};  // 资源视图标签
    struct Value {};  // 价值视图标签

    struct EntryModifier  // 修改容器数据的函数对象
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
    // 增加一条访问记录
    void AddAccessRecord(const AccessRecord& record);

    // 更新一条访问记录
    void UpdateAccessRecord(const AccessRecord& new_record, const AccessRecord& old_record);

    // 删除没有价值资源的线程
    void RemoveResource();

    // 修改资源价值
    void ModifyResourceValue(const ResourceId& resource, hot_value_t value);

    // 资源价值发生改变的回调函数
    void OnResourceValueChanged(const ResourceId& resource,
                                hot_value_t new_value,
                                hot_value_t old_value);

private:
    uint32 remove_resource_interval_ = 100;  // 删除没有价值资源的时间间隔

    mutable boost::mutex mutex_;
    boost::shared_ptr<HotValueCalculator> calculator_;
    boost::shared_ptr<boost::thread> remove_resource_thread_;  // 删除没有价值资源的线程

	AccessRecordDatabase access_records_; // 访问记录容器

    multi_index_container<
        Entry,
        indexed_by<
            hashed_unique<tag<Resource>, member<Entry, ResourceId, &Entry::resource> >,
            ordered_non_unique<tag<Value>, member<Entry, hot_value_t, &Entry::value>, std::greater<hot_value_t> >
        >
    > hot_values_;  // 保存资源价值的容器
};

}  // namespace BroadCache

#endif  // HEADER_HOT_ALGORITHM
