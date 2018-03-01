/*##############################################################################
 * �ļ���   : access_record_database.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.12.05
 * �ļ����� : AccessRecordDatabase��ʵ���ļ� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "access_record_database.hpp"

#include <boost/filesystem.hpp>

#include "bc_util.hpp"
#include "serialize.hpp"
#include "ugs_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: ��ʼ���˶���
 * ��  ��:
 * ����ֵ: ��ʼ���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool AccessRecordDatabase::Init()
{
    LoadDump();  // ����dump�ļ�
    dump_thread_.reset(new boost::thread(boost::bind(&AccessRecordDatabase::WriteDump, this)));

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ���ʼ�¼���Ӻ�Ļص�����
 * ��  ��:
 * ����ֵ: ���ʼ�¼���Ӻ�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
AccessRecordDatabase::RecordAddedHandler
    AccessRecordDatabase::GetRecordAddedHandler() const
{
    return record_added_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���÷��ʼ�¼���Ӻ�Ļص�����
 * ��  ��: [in] handler ���ʼ�¼���Ӻ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::SetRecordAddedHandler(const RecordAddedHandler& handler)
{
    record_added_handler_ = handler;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ���ʼ�¼���º�Ļص�����
 * ��  ��:
 * ����ֵ: ���ʼ�¼���º�Ļص�����
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
AccessRecordDatabase::RecordUpdatedHandler
    AccessRecordDatabase::GetRecordUpdatedHandler() const
{
    return record_updated_handler_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���÷��ʼ�¼���º�Ļص�����
 * ��  ��: [in] handler ���ʼ�¼���º�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::SetRecordUpdatedHandler(const RecordUpdatedHandler& handler)
{
    record_updated_handler_ = handler;
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
void AccessRecordDatabase::RemoveResource(const ResourceId& resource)
{
    boost::lock_guard<decltype(mutex_)> lock(mutex_);
    database_.erase(resource); 
}

/*------------------------------------------------------------------------------
 * ��  ��: дdump�ļ����߳�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::WriteDump() const
{
    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::seconds(dump_interval_));

        std::string file_name = dump_file_name_ + '~';  // ��ʱ�ļ���
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
            // �Ȼ�ȡдһ����Դ��¼�����ڴ�ռ�Ĵ�С
            SizeHelper size_helper;
            size_helper & resource_entry;
            
            uint32 bytes_needed = uint32(sizeof(uint32) + size_helper.value());
            if (bytes_used + bytes_needed > kBufSize)
            {
                dump_file.write(buf, bytes_used);
                bytes_used = 0;
            }

            // ����Դ��¼���е��ڴ�
            Serializer serializer(buf + bytes_used);
            serializer & uint32(size_helper.value()) & resource_entry;

            bytes_used += bytes_needed;
        }
        mutex_.unlock();

        LOG(INFO) << "dump time used : " << time(nullptr) - old_time << " s\n"; 

        dump_file.write(buf, bytes_used);
        dump_file.close();

        // ����ʱ�ļ��滻
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
 * ��  ��: ����dump�ļ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
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
        // ����һ����Դ��¼�Ĵ�С
        uint32 size = 0;
        dump_file.read(
                reinterpret_cast<char*>(&size), sizeof(size));
        auto bytes_read = dump_file.gcount();
        if ((bytes_read != sizeof(size)) || (size == 0))
        {
            break;
        }

        // �����ڴ�ռ�
        buf.resize(size); 

        // ��ȡһ����Դ��¼
        dump_file.read(&buf[0], size);
        bytes_read = dump_file.gcount();

        if (bytes_read != size)
        {
            LOG(INFO) << "Dump file format error.";
            LOG(INFO) << "Size : " << size;
            LOG(INFO) << "Bytes-read : " << bytes_read;
            break;
        }

        // �����л���Դ��¼
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
 * ��  ��: ����һ����Դ���ʼ�¼
 * ��  ��: [in] record ��Դ���ʼ�¼
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.05
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void AccessRecordDatabase::AddAccessRecord(const AccessRecord& record)
{
    mutex_.lock();

    auto& peers = database_[record.resource];  // ����Դ�ķ�����
    address_t address = static_cast<address_t>(record.peer.ip);

    auto i = peers.find(address);
    if (i == peers.end())  // ������֮ǰû�з��ʹ�����Դ�����һ���µļ�¼
    {
        peers.insert(std::make_pair(address, record.access_time));
        mutex_.unlock();

        if (record_added_handler_)
        {
            record_added_handler_(record);
        }
    }
    else  // ������֮ǰ���ʹ�����Դ�����¼�¼
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
