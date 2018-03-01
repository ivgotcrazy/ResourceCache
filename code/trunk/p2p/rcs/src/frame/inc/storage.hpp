/*#############################################################################
 * 文件名   : storage.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : Storage相关声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描述：IO操作抽象接口
 * 作者：teck_zhou
 * 时间：2013/08/21
 *****************************************************************************/
class StorageInterface
{
public:
	virtual ~StorageInterface(){}
	
	virtual bool Initialize() = 0;
	
	virtual bool Read(char* buf, uint64 slot, uint64 offset, uint64 size) = 0;
	virtual bool Write(const char* buf, uint64 slot, uint64 offset, uint64 size) = 0;
	
	virtual bool Remove() = 0;

	//TODO: 后续优化点
	virtual bool ReadV() = 0;
	virtual bool WriteV() = 0;
};

/******************************************************************************
 * 描述：IO操作的文件实现
 * 作者：teck_zhou
 * 时间：2013/08/21
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

	// 单文件读写
	bool ReadWriteSingleFileImpl(char* buf, uint64 slot, uint64 offset, uint64 size, StorageIoOper oper);

private:
	TorrentWP torrent_;

	boost::scoped_ptr<File> torrent_file_;

	boost::mutex file_mutex_;
};

}


#endif