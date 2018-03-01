/*#############################################################################
 * 文件名   : io_oper_manager.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月22日
 * 文件描述 : IoOperManager类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PIECE_MANAGER
#define HEADER_PIECE_MANAGER

#include "depend.hpp"
#include "disk_io_job.hpp"
#include "storage.hpp"

namespace BroadCache
{

class BencodeEntry;
class PeerRequest;
class DiskIoThread;
class FilePool;
class TorrentFile;
class File;
class Session;
class Torrent;

/******************************************************************************
 * 描述: 负责所有磁盘IO操作
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
class IoOperManager : public boost::noncopyable
{
public:	
	IoOperManager(const TorrentSP& torrent, DiskIoThread& io_thread);
	~IoOperManager();

	bool Init();
	void RemoveAllFiles();

	// 异步读写torrent资源数据
	void AsyncRead(const PeerRequest& request, char* buf, IoJobHandler handler);
	void AsyncWrite(const PeerRequest& request, char* buf, IoJobHandler handler);

	// 同步读写torrent资源数据
	int  ReadData(char* buf, int piece_index, int offset, int size);
	int  WriteData(char* buf, int piece_index, int offset, int size);

	// 异步读写fast-resume数据
	void AsyncReadFastResume(IoJobHandler handler);
	void AsyncWriteFastResume(IoJobHandler handler, const std::string& data);
	
	// 同步读写fast-resume数据
	bool ReadFastResume(std::string& data);
	bool WriteFastResume(const std::string& data);

	// 异步读写metadata数据
	void AsyncReadMetadata(IoJobHandler handler);
	void AsyncWriteMetadata(IoJobHandler handler, const std::string& data);

	// 同步读写metadata数据
	bool ReadMetadata(std::string& data);
	bool WriteMetadata(const std::string& data);

	// 异步校验piece的info-hash
	void AsyncVerifyPieceInfoHash(IoJobHandler handler, uint64 piece);

    TorrentWP torrent() { return torrent_; }

private:
	bool ReadFile(File* file, std::string& data);
	bool WriteFile(File* file, const std::string& data);

	bool ReadFileImpl(File* file, char* buf, uint64 len);
	bool WriteFileImpl(File* file, const char* buf, uint64 len);

private:
	TorrentWP torrent_;
    
	DiskIoThread& io_thread_;

	// 负责fast-resume和metadata文件读写
	boost::scoped_ptr<File> fast_resume_file_;
	boost::scoped_ptr<File> metadata_file_;

	// 负责资源文件读写
	boost::scoped_ptr<FileStorage> file_storage_;

	fs::path torrent_path_;
};

}


#endif
