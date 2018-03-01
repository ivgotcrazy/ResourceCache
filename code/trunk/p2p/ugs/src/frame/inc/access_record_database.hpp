/*##############################################################################
 * �ļ���   : access_record_database.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.12.05
 * �ļ����� : ��AccessRecordDatabase�������ļ�
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
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
 * ��  ��: ��Դ���ʼ�¼���ݿ�
 * ��  ��: rosan
 * ʱ  ��: 2013.12.05
 ******************************************************************************/
class AccessRecordDatabase
{
public:
    typedef boost::function<void(const AccessRecord&)> RecordAddedHandler;
    typedef boost::function<void(const AccessRecord&, const AccessRecord&)> RecordUpdatedHandler;

public:
    // ��ʼ��
    bool Init();

    // ����һ����Դ���ʼ�¼
    void AddAccessRecord(const AccessRecord& record);

    // ɾ��һ����Դ
    void RemoveResource(const ResourceId& resource);

    RecordAddedHandler GetRecordAddedHandler() const;
    void SetRecordAddedHandler(const RecordAddedHandler& handler);

    RecordUpdatedHandler GetRecordUpdatedHandler() const;
    void SetRecordUpdatedHandler(const RecordUpdatedHandler& handler);

private:
    typedef uint32 address_t;  // IPv4��ַ

private:
    // ����dump�ļ�
    void LoadDump();

    // дdump�ļ����߳�
    void WriteDump() const;

private:
    uint32 dump_interval_ = 60;  // дdump�ļ���ʱ����
    std::string dump_file_name_ = "access_record.dmp";  // dump�ļ���
    mutable boost::shared_mutex mutex_;
    boost::shared_ptr<boost::thread> dump_thread_;  // дdump�ļ��߳�
    boost::unordered_map<ResourceId, boost::unordered_map<address_t, time_t> > database_;  // ��Դ���ʼ�¼����
    RecordAddedHandler record_added_handler_;  // ����һ����Դ���ʼ�¼�Ļص�����
    RecordUpdatedHandler record_updated_handler_;  // ��Դ���ʼ�¼����ʱ�Ļص����� 
};

}  // namespace BroadCache

#endif  // HEADER_ACCESS_RECORD_DATABASE
