/*##############################################################################
 * 文件名   : access_record_database.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.12.05
 * 文件描述 : AccessRecordDatabase的实现文件 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "access_record_database.hpp"

#include <boost/filesystem.hpp>

#include "bc_util.hpp"
#include "serialize.hpp"
#include "ugs_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 初始化此对象
 * 参  数:
 * 返回值: 初始化是否成功
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool AccessRecordDatabase::Init()
{
    LoadDump();  // 加载dump文件
    dump_thread_.reset(new boost::thread(boost::bind(&AccessRecordDatabase::WriteDump, this)));

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取访问记录增加后的回调函数
 * 参  数:
 * 返回值: 访问记录增加后的回调函数
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
AccessRecordDatabase::RecordAddedHandler
    AccessRecordDatabase::GetRecordAddedHandler() const
{
    return record_added_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置访问记录增加后的回调函数
 * 参  数: [in] handler 访问记录增加后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::SetRecordAddedHandler(const RecordAddedHandler& handler)
{
    record_added_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取访问记录更新后的回调函数
 * 参  数:
 * 返回值: 访问记录更新后的回调函数
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
AccessRecordDatabase::RecordUpdatedHandler
    AccessRecordDatabase::GetRecordUpdatedHandler() const
{
    return record_updated_handler_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置访问记录更新后的回调函数
 * 参  数: [in] handler 访问记录更新后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::SetRecordUpdatedHandler(const RecordUpdatedHandler& handler)
{
    record_updated_handler_ = handler;
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
void AccessRecordDatabase::RemoveResource(const ResourceId& resource)
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);
    database_.erase(resource); 
}

/*------------------------------------------------------------------------------
 * 描  述: 写dump文件的线程
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::WriteDump() const
{
    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::seconds(dump_interval_));

        std::string file_name = dump_file_name_ + '~';  // 临时文件名
        std::ofstream dump_file(file_name, std::ios_base::binary);

        if (!dump_file.is_open())
        {
            LOG(INFO) << "Fail to open dump file to write.";
            return ;
        }

        static const uint32 kBufSize = 65536 * 16;
        char buf[kBufSize];

        uint32 bytes_used = 0;

        mutex_.lock();
        time_t old_time = time(nullptr);
        FOREACH(resource_entry, database_)
        {
            // 先获取写一条资源记录所需内存空间的大小
            SizeHelper size_helper;
            size_helper & resource_entry;
            
            uint32 bytes_needed = uint32(sizeof(uint32) + size_helper.value());
            if (bytes_used + bytes_needed > kBufSize)
            {
                dump_file.write(buf, bytes_used);
                bytes_used = 0;
            }

            // 将资源记录序列到内存
            Serializer serializer(buf + bytes_used);
            serializer & uint32(size_helper.value()) & resource_entry;

            bytes_used += bytes_needed;
        }
        mutex_.unlock();

        LOG(INFO) << "dump time used : " << time(nullptr) - old_time << " s\n"; 

        dump_file.write(buf, bytes_used);
        dump_file.close();

        // 将临时文件替换
        boost::filesystem::rename(boost::filesystem::path(file_name),
            boost::filesystem::path(dump_file_name_));

        LOG(INFO) << "dump time used : " << time(nullptr) - old_time << " s\n"; 

        size_t i = 0;
        std::ofstream ofs("log.txt");
        FOREACH(resource_entry, database_)
        {
            ofs << std::setw(8) << i++ << " " << std::setw(10) << resource_entry.first.info_hash
                << "  " << std::setw(12) << resource_entry.second.size() << '\n';
        }
        ofs.close();
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 加载dump文件
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.12.05
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::LoadDump()
{
    std::ifstream dump_file(dump_file_name_, std::ios_base::binary);

    if (!dump_file.is_open())
    {
        LOG(INFO) << "Fail to open dump file to read.";
        return ;
    }

    std::vector<char> buf;
    while (!dump_file.eof())
    {
        // 读入一条资源记录的大小
        uint32 size = 0;
        dump_file.read(
                reinterpret_cast<char*>(&size), sizeof(size));
        auto bytes_read = dump_file.gcount();
        if ((bytes_read != sizeof(size)) || (size == 0))
        {
            break;
        }

        // 分配内存空间
        buf.resize(size); 

        // 读取一条资源记录
        dump_file.read(&buf[0], size);
        bytes_read = dump_file.gcount();

        if (bytes_read != size)
        {
            LOG(INFO) << "Dump file format error.";
            LOG(INFO) << "Size : " << size;
            LOG(INFO) << "Bytes-read : " << bytes_read;
            break;
        }

        // 反序列化资源记录
        typename decltype(database_)::value_type entry;
        Unserializer unserializer(&buf[0]);
        unserializer & entry;

        database_.insert(entry);
    }

    LOG(INFO) << "Read dump file finished.";

    if (!record_added_handler_)
        return ;

    AccessRecord record;
    FOREACH(resource_entry, database_)
    {
        record.resource = resource_entry.first;
        FOREACH(address_entry, resource_entry.second)
        {
            record.peer.ip = address_entry.first;
            record.access_time = address_entry.second;
            record_added_handler_(record);
        }
    }
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
void AccessRecordDatabase::AddAccessRecord(const AccessRecord& record)
{
    mutex_.lock();

    auto& peers = database_[record.resource];  // 此资源的访问者
    address_t address = static_cast<address_t>(record.peer.ip);

    auto i = peers.find(address);
    if (i == peers.end())  // 访问者之前没有访问过此资源，添加一条新的记录
    {
        peers.insert(std::make_pair(address, record.access_time));
        mutex_.unlock();

        if (record_added_handler_)
        {
            record_added_handler_(record);
        }
    }
    else  // 访问者之前访问过此资源，更新记录
    {
        AccessRecord old_record = record;
        old_record.access_time = i->second;

        i->second = record.access_time;
        mutex_.unlock();

        if (record_updated_handler_)
        {
            record_updated_handler_(record, old_record);
        }
    }
}

}  // namespace BroadCache
