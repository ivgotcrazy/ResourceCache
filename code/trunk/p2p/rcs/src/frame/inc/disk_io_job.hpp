/*#############################################################################
 * 文件名   : disk_io_job.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年9月2日
 * 文件描述 : DiskIoJob声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_DISK_IO_JOB
#define HEADER_DISK_IO_JOB

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"

namespace BroadCache
{

class DiskIoThread;
class PieceManager;

// Job处理回调函数，Job创建者负责实现
typedef boost::function<void(bool, const DiskIoJobSP&)> IoJobHandler;

/******************************************************************************
 * 描述：所有IO操作接口
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
INTERFACE DiskIoJob 
{
	DiskIoJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler);

	virtual void Do(DiskIoThread& io_thread) = 0;
	virtual ~DiskIoJob() {}

	io_service&  work_ios;
	TorrentSP	 torrent;
	IoJobHandler callback;
};

/******************************************************************************
 * 描述: 从磁盘读取Torrent资源数据
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
struct ReadDataJob : public DiskIoJob,
	                 public boost::enable_shared_from_this<ReadDataJob>
{
	ReadDataJob(io_service& ios, 
		        const TorrentSP& t, 
				const IoJobHandler& handler, 
				char* buffer, 
				uint64 piece, 
				uint64 offset, 
				uint64 length);

	void Do(DiskIoThread& io_thread);
	
	uint64 piece;	// 读取数据的piece索引
	uint64 offset;	// 读取数据位置偏移
	uint64 len;		// 读取数据的长度
	char*  buf;		// 读取数据缓冲区
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const ReadDataJob& job)
{
	stream << "[" << job.piece << ":" << job.offset << ":" << job.len << "]";
	return stream;
}

/******************************************************************************
 * 描述: 将Torrent资源数据写入磁盘
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
struct WriteDataJob : public DiskIoJob,
	                  public boost::enable_shared_from_this<WriteDataJob>
{
	WriteDataJob(io_service& ios, 
		         const TorrentSP& t, 
				 const IoJobHandler& handler, 
				 char* buffer, 
				 uint64 piece, 
				 uint64 offset, 
				 uint64 length);

	void Do(DiskIoThread& io_thread);

	uint64	piece;	// 写入数据的piece索引
	uint64	offset;	// 写入数据位置偏移
	uint64	len;	// 写入数据长度
	char*	buf;	// 写入数据缓冲区
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const WriteDataJob& job)
{
	stream << "[" << job.piece << ":" << job.offset << ":" << job.len << "]";
	return stream;
}

/******************************************************************************
 * 描述: 从磁盘读取FastResume数据
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
struct WriteResumeJob : public DiskIoJob,
	                    public boost::enable_shared_from_this<WriteResumeJob>
{
	WriteResumeJob(io_service& ios, 
		           const TorrentSP& t, 
				   const IoJobHandler& handler, 
				   const std::string& data);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string fast_resume;
};

/******************************************************************************
 * 描述: 将FastResume数据写入磁盘
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
struct ReadResumeJob : public DiskIoJob,
	                   public boost::enable_shared_from_this<ReadResumeJob>
{
	ReadResumeJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string fast_resume;
};

/******************************************************************************
 * 描述: 从磁盘读取metadata数据
 * 作者：teck_zhou
 * 时间：2013/10/18
 *****************************************************************************/
struct WriteMetadataJob : public DiskIoJob,
	                      public boost::enable_shared_from_this<WriteMetadataJob>
{
	WriteMetadataJob(io_service& ios, 
		             const TorrentSP& t, 
					 const IoJobHandler& handler, 
					 const std::string& data);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string metadata;
};

/******************************************************************************
* 描述: 向磁盘写入metadata数据
* 作者：teck_zhou
* 时间：2013/10/18
 *****************************************************************************/
struct ReadMetadataJob : public DiskIoJob,
	                     public boost::enable_shared_from_this<ReadMetadataJob>
{
	ReadMetadataJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string metadata;
};

/******************************************************************************
* 描述: 校验piece的hash
* 作者：teck_zhou
* 时间：2013/11/06
 *****************************************************************************/
struct VerifyPieceInfoHashJob : public DiskIoJob,
	public boost::enable_shared_from_this<VerifyPieceInfoHashJob>
{
	VerifyPieceInfoHashJob(io_service& ios, 
		                   const TorrentSP& t, 
						   const IoJobHandler& handler, 
						   uint64 piece);

	virtual void Do(DiskIoThread& io_thread) override;

	uint64 piece_index;
};

}

#endif
