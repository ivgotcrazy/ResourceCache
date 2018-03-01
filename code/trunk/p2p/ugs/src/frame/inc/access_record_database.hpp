/*##############################################################################
 * 文件名   : access_record_database.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.12.05
 * 文件描述 : 类AccessRecordDatabase的声明文件
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_ACCESS_RECORD_DATABASE
#define HEADER_ACCESS_RECORD_DATABASE

#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>

#include "singleton.hpp"
#include "ugs_typedef.hpp"
#include "ugs_util.hpp"
#include "timer.hpp"

namespace BroadCache
{

/*******************************************************************************
 * 描  述: 资源访问记录数据库
 * 作  者: rosan
 * 时  间: 2013.12.05
 ******************************************************************************/
class AccessRecordDatabase
{
public:
    typedef boost::function<void(const AccessRecord&)> RecordAddedHandler;
    typedef boost::function<void(const AccessRecord&, const AccessRecord&)> RecordUpdatedHandler;

public:
    // 初始化
    bool Init();

    // 增加一条资源访问记录
    void AddAccessRecord(const AccessRecord& record);

    // 删除一个资源
    void RemoveResource(const ResourceId& resource);

    RecordAddedHandler GetRecordAddedHandler() const;
    void SetRecordAddedHandler(const RecordAddedHandler& handler);

    RecordUpdatedHandler GetRecordUpdatedHandler() const;
    void SetRecordUpdatedHandler(const RecordUpdatedHandler& handler);

private:
    typedef uint32 address_t;  // IPv4地址

private:
    // 加载dump文件
    void LoadDump();

    // 写dump文件的线程
    void WriteDump() const;

private:
    uint32 dump_interval_ = 60;  // 写dump文件的时间间隔
    std::string dump_file_name_ = "access_record.dmp";  // dump文件名
    mutable boost::shared_mutex mutex_;
    boost::shared_ptr<boost::thread> dump_thread_;  // 写dump文件线程
    boost::unordered_map<ResourceId, boost::unordered_map<address_t, time_t> > database_;  // 资源访问记录容器
    RecordAddedHandler record_added_handler_;  // 增加一条资源访问记录的回调函数
    RecordUpdatedHandler record_updated_handler_;  // 资源访问记录更新时的回调函数 
};

}  // namespace BroadCache

#endif  // HEADER_ACCESS_RECORD_DATABASE
