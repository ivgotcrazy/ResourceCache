/*#############################################################################
 * 文件名   : io_oper_manager.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月22日
 * 文件描述 : IoOperManager类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "io_oper_manager.hpp"
#include "disk_io_job.hpp"
#include "disk_io_thread.hpp"
#include "bc_assert.hpp"
#include "peer_request.hpp"
#include "session.hpp"
#include "torrent.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: [in] session 所属Session
 *         [in] torrent 所属Torrent
 *         [in] torrrent_file Torrent所包含文件对象
 *         [in] resume_file FastResume文件
 *         [in] io_thread IO调度对象
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
IoOperManager::IoOperManager(const TorrentSP& torrent, DiskIoThread& io_thread)
    : torrent_(torrent), io_thread_(io_thread)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
IoOperManager::~IoOperManager()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::Init()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	std::string info_hash_str(torrent->info_hash()->to_string());
	
	torrent_path_ = torrent->save_path() / fs::path(info_hash_str);
	
	// 如果torrent目录不存在，则先创建目录
	if (!fs::exists(torrent_path_))
	{
		if (!fs::create_directory(torrent_path_))
		{
			LOG(ERROR) << "Fail to create torrent directory | " << torrent_path_;
			return false;
		}
	}

	// 创建资源文件IO对象
	file_storage_.reset(new FileStorage(torrent));
	if (!file_storage_->Initialize())
	{
		LOG(ERROR) << "Fail to initialize file storage";
		return false;
	}

	// 打开/创建fast-resume文件
	fs::path fast_resume_path = torrent_path_ / fs::path(info_hash_str + ".fastresume");
	fast_resume_file_.reset(new File(fast_resume_path, File::in | File::out));
	if (!fast_resume_file_->IsOpen())
	{
		LOG(ERROR) << "Fail to open fast-resume file | " << info_hash_str;
		return false;
	}

	// 打开/创建metadata文件
	fs::path metadata_path = torrent_path_ / fs::path(info_hash_str + ".metadata");
	metadata_file_.reset(new File(metadata_path, File::in | File::out));
	if (!fast_resume_file_->IsOpen())
	{
		LOG(ERROR) << "Fail to open metadata file | " << info_hash_str;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 删除所有文件
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void IoOperManager::RemoveAllFiles()
{
	// 删除资源文件
	if (file_storage_ && !file_storage_->Remove())
	{
		LOG(ERROR) << "Fail to remove resource file";
	}

	// 删除fast-resume文件
	if (fast_resume_file_ && !fast_resume_file_->Remove())
	{
		LOG(ERROR) << "Fail to remove fast-resume file";
	}

	// 删除metadata文件
	if (metadata_file_ && !metadata_file_->Remove())
	{
		LOG(ERROR) << "Fail to remove metadata file";
	}

	// 删除外层目录
	if (-1 == ::remove(torrent_path_.string().c_str()))
	{
		LOG(ERROR) << "Fail to remove torrent directory";
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 异步读
 * 参  数: [in] request 数据请求
 *         [in] handler 回调函数
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncRead(const PeerRequest& request, char* buf, IoJobHandler handler)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new ReadDataJob(torrent->session().work_ios(), torrent, 
		handler, buf, request.piece, request.start, request.length));

    io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * 描  述: 异步写
 * 参  数: [in] request 数据请求
 *         [in] buf 缓冲区
 *         [in] handler 回调函数
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncWrite(const PeerRequest& request, char* buf, IoJobHandler handler)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

    DiskIoJobSP job(new WriteDataJob(torrent->session().work_ios(), torrent, 
		handler, buf, request.piece, request.start, request.length));

    io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * 描  述: 读取数据实现
 * 参  数: [in] buf 数据缓冲区
 *         [in] piece_index	piece索引
 *         [in] offset 片内偏移
 *         [in] size 读取数据的长度
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
int IoOperManager::ReadData(char* buf, int piece_index, int offset, int size)
{
	BC_ASSERT(buf);

    // 当前piece_index和slot是一一对应的

    bool ret = file_storage_->Read(buf, 
						     static_cast<std::size_t>(piece_index), 
						     static_cast<std::size_t>(offset), 
						     static_cast<std::size_t>(size));

	if (!ret)
	{
		LOG(ERROR) << "Fail to read data | piece: " << piece_index
			       << " offset: " << offset << ", size: " << size;
		return 0;
	}

	return size;
}

/*-----------------------------------------------------------------------------
 * 描  述: 写数据实现
 * 参  数: [in] buf	数据缓冲区
 *         [in] piece_index	piece索引
 *         [in] offset 片内偏移
 *         [in] size 读取数据的长度
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
int IoOperManager::WriteData(char* buf, int piece_index, int offset, int size)
{
    BC_ASSERT(buf);
    BC_ASSERT(size > 0);
    BC_ASSERT(offset >= 0);
    BC_ASSERT(piece_index >= 0/*&& piece_index < GetTorrentFile()*/);

    bool ret = file_storage_->Write(buf, 
		                      static_cast<std::size_t>(piece_index), 
							  static_cast<std::size_t>(offset), 
							  static_cast<std::size_t>(size));
    if (!ret)
    {
        LOG(ERROR) << "Fail to write data | piece: " << piece_index
			       << " offset: " << offset << ", size: " << size;
        return 0;
    }

    return size;
}

/*-----------------------------------------------------------------------------
 * 描  述: 异步读取fast-resume数据
 * 参  数: [in] handler 回调函数
 *         [in] buf 读缓冲区
 *         [out] len 读取数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncReadFastResume(IoJobHandler handler)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new ReadResumeJob(torrent->session().work_ios(), torrent, handler));

    io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * 描  述: 异步保存fast-resume数据
 * 参  数: [in] handler 回调函数
 *         [in] buf 数据缓冲区
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncWriteFastResume(IoJobHandler handler, const std::string& data)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new WriteResumeJob(torrent->session().work_ios(), torrent, handler, data));

    io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * 描  述: 将fastresume数据写入文件
 * 参  数: [in] data 写入数据
 * 返回值: 写入是否成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteFastResume(const std::string& data)
{
	return WriteFile(fast_resume_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件中读取fastresume
 * 参  数: [out] buf 读取fast-resume数据的缓冲区
 * 返回值: 读取是否成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::ReadFastResume(std::string& data)
{
	return ReadFile(fast_resume_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * 描  述: 异步读取metadata数据
 * 参  数: [in] handler 回调函数
 *         [in] buf 读缓冲区
 *         [out] len 读缓冲区长度
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncReadMetadata(IoJobHandler handler)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new ReadMetadataJob(torrent->session().work_ios(), torrent, handler));

	io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * 描  述: 异步读取metadata数据
 * 参  数: [in] handler 回调函数
 *         [in] buf 读缓冲区
 *         [out] len 读缓冲区长度
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncWriteMetadata(IoJobHandler handler, const std::string& data)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new WriteMetadataJob(torrent->session().work_ios(), torrent, handler, data));

	io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * 描  述: 将metadata数据写入文件
 * 参  数: [in] buf 数据指针
 *         [in] len 数据长度
 * 返回值: 写入是否成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteMetadata(const std::string& data)
{
	return WriteFile(metadata_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件中读取metadata
 * 参  数: [out] buf 读取metadata数据的缓冲区
 *         [in] len 数据的缓冲区长度
 * 返回值: 读取是否成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::ReadMetadata(std::string& data)
{
	return ReadFile(metadata_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件中读取数据
 * 参  数: [in] file_path 文件路径
 *         [out] data 数据缓冲区
 * 返回值: 读取是否成功
 * 修  改:
 *   时间 2013年11月11日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::ReadFile(File* file, std::string& data)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	error_code ec;
	uint64 file_size = file->GetSize(ec);
	if (ec)
	{
		LOG(INFO) << "Fail to get file size";
		return false;
	}

	data.resize(file_size);

	return ReadFileImpl(file, const_cast<char*>(data.c_str()), file_size);
}

/*-----------------------------------------------------------------------------
 * 描  述: 向文件中写入数据
 * 参  数: [in] file_path 文件路径
 *         [in] data 数据缓冲区
 * 返回值: 写入是否成功
 * 修  改:
 *   时间 2013年11月11日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteFile(File* file, const std::string& data)
{
	return WriteFileImpl(file, data.c_str(), data.size());
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件中读取数据
 * 参  数: [out] buf 读取metadata数据的缓冲区
 *         [in] len 数据的缓冲区长度
 * 返回值: 读取是否成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::ReadFileImpl(File* file, char* buf, uint64 len)
{
	BC_ASSERT(file);

	if(!file->IsOpen())
	{
		LOG(ERROR) << "File is not opened";
		return false;
	}

	error_code error;

	file->Seek(0, File::begin, error);
	if(error)
	{
		LOG(ERROR) << "Seek file error | " << error.message();
		return false;
	} 

	uint64 bytes_read = file->Read(buf, len, error);
	if(error || (bytes_read != len))
	{
		LOG(ERROR) << "Read file error | error: " << error.message() 
			       << " len: " << len << " read size: " << bytes_read;
		return false;
	} 

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 将数据写入文件
 * 参  数: [in] buf 数据指针
 *         [in] len 数据长度
 * 返回值: 写入是否成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteFileImpl(File* file, const char* buf, uint64 len)
{
	if(!file->IsOpen()) 
	{
		LOG(ERROR) << "File is not opened";
		return false;
	}

	error_code error;

	file->SetSize(0, error); //设置文件大小
	if(error)
	{
		LOG(ERROR) << "Set file size error | " << error.message();
		return false;
	}

	file->Seek(0, File::begin, error); //设置文件指针
	if(error)
	{
		LOG(ERROR) << "Seek file error | " << error.message();
		return false;
	}

	uint64 bytes_written = file->Write(buf, len, error); //写入数据
	if(error || (bytes_written != len))
	{
		LOG(ERROR) << "Write file error | error: " << error.message() 
			       << " len: " << len << " read size: " << bytes_written;
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 异步校验piece的info-hash
 * 参  数: [in] handler 回调函数
 *         [in] piece piece索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月6日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void IoOperManager::AsyncVerifyPieceInfoHash(IoJobHandler handler, uint64 piece)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new VerifyPieceInfoHashJob(torrent->session().work_ios(), torrent, handler, piece));

	io_thread_.AddJob(job);
}

}
