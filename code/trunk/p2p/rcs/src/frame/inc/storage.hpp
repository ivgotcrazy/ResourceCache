/*#############################################################################
 * �ļ���   : storage.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : Storage�������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEAER_STORAGE
#define HEAER_STORAGE

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "torrent_file.hpp"

namespace BroadCache
{

class Torrent;
class TorrentFile;
class FilePool;
class File;

namespace fs = boost::filesystem;

/******************************************************************************
 * ������IO��������ӿ�
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/08/21
 *****************************************************************************/
class StorageInterface
{
public:
	virtual ~StorageInterface(){}
	
	virtual bool Initialize() = 0;
	
	virtual bool Read(char* buf, uint64 slot, uint64 offset, uint64 size) = 0;
	virtual bool Write(const char* buf, uint64 slot, uint64 offset, uint64 size) = 0;
	
	virtual bool Remove() = 0;

	//TODO: �����Ż���
	virtual bool ReadV() = 0;
	virtual bool WriteV() = 0;
};

/******************************************************************************
 * ������IO�������ļ�ʵ��
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/08/21
 *****************************************************************************/
class FileStorage : public StorageInterface, boost::noncopyable
{
public:
	FileStorage(const TorrentSP& t);
	~FileStorage();

	virtual bool Initialize() override;

	virtual bool Read(char* buf, uint64 slot, uint64 offset, uint64 size) override;
	virtual bool Write(const char* buf, uint64 slot, uint64 offset, uint64 size) override;

	virtual bool Remove() override;

	virtual bool ReadV() override { return false; }
	virtual bool WriteV() override { return false; }
	
private:
	typedef std::vector<FileEntry>::const_iterator FileEntryConstIter;
	typedef boost::function<uint64(File*, char*, uint64, error_code&)> StorageIoOper;

	// ���ļ���д
	bool ReadWriteSingleFileImpl(char* buf, uint64 slot, uint64 offset, uint64 size, StorageIoOper oper);

private:
	TorrentWP torrent_;

	boost::scoped_ptr<File> torrent_file_;

	boost::mutex file_mutex_;
};

}


#endif