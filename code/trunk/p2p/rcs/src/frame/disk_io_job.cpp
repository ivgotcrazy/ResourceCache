/*#############################################################################
 * 文件名   : disk_io_job.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月22日
 * 文件描述 : DiskIoJob实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "disk_io_job.hpp"
#include "disk_io_thread.hpp"
#include "io_oper_manager.hpp"
#include "torrent.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 结构体构造参数，略
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DiskIoJob::DiskIoJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler) 
	: work_ios(ios), torrent(t), callback(handler)
{
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 结构体构造参数，略
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
ReadDataJob::ReadDataJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler, 
	char* buffer, uint64 piece, uint64 offset, uint64 length)
	: DiskIoJob(ios, t, handler)
	, piece(piece)
	, offset(offset)
	, len(length)
	, buf(buffer)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 具体执行读数据处理
 * 参  数: [in] io_thread 需要调用DiskIoThread提供的接口
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void ReadDataJob::Do(DiskIoThread &io_thread)
{
	ReadDataJobSP read_data = shared_from_this();
	bool result = io_thread.ReadData(read_data);
	if (!result)
	{
		LOG(WARNING) << "Fail to read data frome disk";
	}
	
	// 回调处理
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 结构体构造参数，略
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
WriteDataJob::WriteDataJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler, 
	char* buffer, uint64 piece, uint64 offset, uint64 length)
	: DiskIoJob(ios, t, handler)
	, piece(piece)
	, offset(offset)
	, len(length)
	, buf(buffer)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 具体执行写入数据处理
 * 参  数: [in] io_thread 需要调用DiskIoThread提供的接口
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void WriteDataJob::Do(DiskIoThread &io_thread)
{
	bool result = io_thread.WriteData(shared_from_this());
	if (!result)
	{
		LOG(WARNING) << "Fail to write data to disk";
	}
	
	// 回调处理
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: WriteResumeJob构造函数
 * 参  数: [in]pm Piece管理指针
 *         [in]handler 此job完成后的回调函数 
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
WriteResumeJob::WriteResumeJob(io_service& ios, 
	const TorrentSP& t, const IoJobHandler& handler, const std::string& data)
    : DiskIoJob(ios, t, handler), fast_resume(data)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 写fast resume信息
 * 参  数: [in]DiskIoThread 未使用
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void WriteResumeJob::Do(DiskIoThread&)
{
    //将fast-resume信息写到文件中
    bool result = torrent->io_oper_manager()->WriteFastResume(fast_resume);
    if (!result) //写入失败
	{
		LOG(WARNING) << "Fail to write fast-resume";
	}
	
	// 回调处理
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: ReadResumeJob构造函数
 * 参  数: [in]pm piece管理器
 *         [out]handler 此job完成后的回调函数
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
ReadResumeJob::ReadResumeJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler)
    : DiskIoJob(ios, t, handler)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件读取fast resume信息 
 * 参  数: [in]DiskIoThread 未使用
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void ReadResumeJob::Do(DiskIoThread&)
{
    //从文件中读取fast-resume用bencode编码后的数据
    bool result = torrent->io_oper_manager()->ReadFastResume(fast_resume);
    if (!result) //读取失败
    { 
		LOG(WARNING) << "Fail to read fast-resume";
    }
	
	// 回调处理
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: WriteMetadataJob构造函数
 * 参  数: [in] pm piece管理器
 *         [in] handler 此job完成后的回调函数
 *         [in] buffer 读取数据缓冲区
 *         [in] length 读取的数据长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
WriteMetadataJob::WriteMetadataJob(io_service& ios, const TorrentSP& t, 
	const IoJobHandler& handler, const std::string& data)
    : DiskIoJob(ios, t, handler)
	, metadata(data)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件读取fast resume信息 
 * 参  数: [in] DiskIoThread 未使用
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void WriteMetadataJob::Do(DiskIoThread&)
{
    //从文件中读取fast-resume用bencode编码后的数据
    bool result = torrent->io_oper_manager()->WriteMetadata(metadata);
    if (!result)
    { 
		LOG(WARNING) << "Fail to write metadata";
    }
	
	// 回调处理
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: ReadMetadataJob构造函数
 * 参  数: [in] pm piece管理器
 *         [in] handler 此job完成后的回调函数
 *         [in] buffer 读取数据缓冲区
 *         [in] length 读取的数据长度
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
ReadMetadataJob::ReadMetadataJob(io_service& ios, const TorrentSP& t, 
	const IoJobHandler& handler)
    : DiskIoJob(ios, t, handler)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件读取fast resume信息 
 * 参  数: [in]DiskIoThread 未使用
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void ReadMetadataJob::Do(DiskIoThread&)
{
    //从文件中读取fast-resume用bencode编码后的数据
    bool result = torrent->io_oper_manager()->ReadMetadata(metadata);
    if (!result) //读取失败
    { 
		LOG(WARNING) << "Fail to read metadata";
    }
	
	// 回调处理
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: [in] pm piece管理器
 *         [in] handler 此job完成后的回调函数
 *         [in] piece piece索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月6日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
VerifyPieceInfoHashJob::VerifyPieceInfoHashJob(io_service& ios, const TorrentSP& t, 
	const IoJobHandler& handler, uint64 piece)
    : DiskIoJob(ios, t, handler)
	, piece_index(piece)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 异步校验piece的info-hash
 * 参  数: [in] io_thread DiskIoThread
 * 返回值:
 * 修  改:
 *   时间 2013年11月6日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void VerifyPieceInfoHashJob::Do(DiskIoThread& io_thread)
{
    bool result = io_thread.VerifyPieceInfoHash(torrent, piece_index);
	if (!result)
	{
		LOG(ERROR) << "Fail to verify piece info-hash | " << piece_index;
	}
	
	// 回调处理
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

}
