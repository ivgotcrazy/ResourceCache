/*#############################################################################
 * 文件名   : storage.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : Storage相关实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bc_typedef.hpp"
#include "file.hpp"
#include "bc_assert.hpp"
#include "storage.hpp"
#include "file_pool.hpp"
#include "torrent.hpp"
#include "info_hash.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: [in] files 包含的文件
           [in] path 文件保存的路径
		   [in] fp 文件池，由Session统一管理
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
FileStorage::FileStorage(const TorrentSP& t) : torrent_(t)
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
FileStorage::~FileStorage()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FileStorage::Initialize()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	std::string info_hash_str(torrent->info_hash()->to_string());

	fs::path torrent_file = torrent->save_path() / fs::path(info_hash_str) / fs::path(info_hash_str);

	// 打开/创建资源文件
	torrent_file_.reset(new File(torrent_file, File::in | File::out));
	if (!torrent_file_->IsOpen())
	{
		LOG(ERROR) << "Fail to open torrent file | " << info_hash_str;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 从文件读取一段数据
 * 参  数: [in] buf	    缓冲区
 *         [in] slot	槽位，当前可以认为就是piece
 *         [in] offset	相对于slot的偏移，即片内偏移
 *         [in] size	读取的数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FileStorage::Read(char* buf, uint64 slot, uint64 offset, uint64 size)
{
	return ReadWriteSingleFileImpl(buf, slot, offset, size, &File::Read);
}

/*-----------------------------------------------------------------------------
 * 描  述: 向文件写入一段数据，需要考虑跨piece和跨文件的写入
 * 参  数: [in] buf	待写入的数据
 *         [in] slot 槽位，当前可以认为就是piece
 *   	   [in] offset 相对于slot的偏移，即片内偏移
 *  	   [in] size 写入的数据长度
 * 返回值: -1	错误
 *         其他	写入的数据大小
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FileStorage::Write(const char* buf, uint64 slot, uint64 offset, uint64 size)
{
	return ReadWriteSingleFileImpl(const_cast<char*>(buf), slot, offset, size, &File::Write);
}

/*-----------------------------------------------------------------------------
 * 描  述: 单文件读写的公共处理流程
 * 参  数: [in] buf		操作的数据
 *         [in] slot	槽位，当前可以认为就是piece
 *   	   [in] offset	相对于slot的偏移，即片内偏移
 *  	   [in] size	数据长度
 *         [in] oper	具体的IO操作
 * 返回值: -1	错误
 *         其他	成功
 * 修  改:
 *   时间 2013年11月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FileStorage::ReadWriteSingleFileImpl(char* buf, uint64 slot, uint64 offset, 
	                                      uint64 size, StorageIoOper oper)
{
	BC_ASSERT(buf);
	BC_ASSERT(torrent_file_);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	// 默认写入的数据不能超过piece边界的
	if (offset + size > torrent->GetPieceSize(slot))
	{
		LOG(ERROR) << "Read/Write data exceed piece boundary |" << " piece: " 
			<< slot << " offset: " << offset << " write: " << size;
		return false;
	}

	uint64 file_offset = slot * torrent->GetCommonPieceSize() + offset;

	// 文件操作需要用锁进行保护
	boost::mutex::scoped_lock lock(file_mutex_);

	error_code ec;

	// 定位文件内操作位置
	uint64 seek_pos = torrent_file_->Seek(file_offset, File::begin, ec);
	if (ec || seek_pos != file_offset)
	{
		LOG(ERROR) << "Fail to seek file | " << ec.message();
		return false;
	}

	uint64 real_types = oper(torrent_file_.get(), buf, size, ec);
	if (ec)
	{
		LOG(ERROR) << "Fail to operate file | " << ec.message();
		return false;
	}

	if (real_types != size)
	{
		LOG(ERROR) << "Fail to read/write data | expect: " 
			<< size << ", actually: " << real_types;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 移除文件
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FileStorage::Remove()
{
	boost::mutex::scoped_lock lock(file_mutex_);

	return torrent_file_->Remove();
}

#if 0
/*-----------------------------------------------------------------------------
 * 描  述: 文件读写的公共处理流程
 * 参  数: [in] buf		操作的数据
 *         [in] slot	槽位，当前可以认为就是piece
 *   	   [in] offset	相对于slot的偏移，即片内偏移
 *  	   [in] size	数据长度
 *         [in] oper	具体的IO操作
 * 返回值: -1	错误
 *         其他	成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FileStorage::ReadWriteImpl(char* buf, uint64 slot, uint64 offset, 
	                            uint64 size, StorageIoOper oper)
{
	BC_ASSERT(buf);

	uint64 file_offset = 0; // 找到文件的文件内偏移
	FileEntryConstIter file_iter = FindFileEntry(slot, offset, &file_offset);
	if (file_iter == files_.End())
	{
		LOG(ERROR) << "Can't find file | piece: " << slot << ", offset: " << offset;
		return false;
	}

	File* file = OpenFile(*file_iter, offset);
	if (nullptr == file)
	{
		return false;
	}

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	uint64 slot_size = torrent->GetPieceSize(slot);
	int64 left_bytes = size;

	// 默认写入的数据不能超过piece边界的
	if (offset + size > slot_size)
	{
		// left_bytes = slot_size - offset;
		LOG(ERROR) << "Read/Write data exceed piece boundary |"
			       << " piece: " << slot
				   << " offset: " << offset
				   << " write: " << left_bytes;
		return false;
	}

	// 文件操作需要用锁进行保护
	boost::mutex::scoped_lock lock(file_mutex_);

	uint64 buf_pos = 0;
	while (left_bytes > 0)
	{
		uint64 rw_bytes = left_bytes;

		// 判断是否会有跨文件操作，如果是则需要逐个操作文件
		if (file_offset + rw_bytes > file_iter->size)
		{
			BC_ASSERT(file_iter->size >= file_offset);
			rw_bytes = file_iter->size - file_offset;
		}

		// 读写文件
		if (rw_bytes > 0)
		{
			error_code ec;

			// 定位文件内操作位置
			uint64 seek_pos = file->Seek(file_offset, File::begin, ec);
			if (ec || seek_pos != file_offset)
			{
				LOG(ERROR) << "Fail to seek file(" << file->file_path()
					<< ") | error_code: " << ec;
				return false;
			}

			uint64 real_types = oper(file, buf + buf_pos, rw_bytes, ec);
			if (ec)
			{
				LOG(ERROR) << "Fail to operate file(" << file->file_path()
					       << ") | error_code: " << ec;
				return false;
			}

			if (rw_bytes != real_types)
			{
				LOG(ERROR) << "Fail to read/write data | expect: " 
					       << rw_bytes << ", actually: " << real_types;
				return false;
			}

			left_bytes -= rw_bytes;
			buf_pos += rw_bytes;
			file_offset += rw_bytes;
		}

		// 如果还有数据没有写入，则应该写入下一个文件
		if (left_bytes > 0)
		{
			++file_iter;
			BC_ASSERT(file_iter != files_.End());

			file = OpenFile(*file_iter, offset);
			if (nullptr == file)
			{
				return false;
			}
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 文件读写的公共处理流程
 * 参  数: [in] slot 槽位，当前可以认为就是piece
 *   	   [in] offset	相对于slot的偏移，即片内偏移
 *  	   [out] file_offset piece内偏移转换为找到文件的文件内偏移
 * 返回值: -1	错误
 *         其他	成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
FileStorage::FileEntryConstIter 
FileStorage::FindFileEntry(uint64 slot, uint64 offset, uint64* file_offset)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return files_.End();

	// 相对于整个资源的偏移
	uint64 resource_offset = slot * torrent->GetCommonPieceSize() + offset;

	FileEntryConstIter file_iter = files_.Begin();
	while (file_iter != files_.End() && resource_offset > file_iter->size)
	{
		resource_offset -= file_iter->size;
		++file_iter;
	}

	if (file_iter != files_.End())
	{
		*file_offset = resource_offset;
	}
	
	return file_iter;
}

/*-----------------------------------------------------------------------------
 * 描  述: 文件读写的公共处理流程
 * 参  数: [in] file_entry 文件
 *         [in] offset 打开后定位
 * 返回值: -1	错误
 *         其他	成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
File* FileStorage::OpenFile(const FileEntry& file_entry, uint64 offset)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return nullptr;

	// 打开文件
	File* file = file_pool_.OpenFile(torrent.get(), file_entry.path, File::out | File::in);
	if (!file)
	{
		LOG(ERROR) << "Fail to open file | " << file_entry.path;
		return nullptr;
	}

	// 定位读写的位置
	error_code ec;
	uint64 pos = file->Seek(offset, File::begin, ec);
	if (pos != offset || ec)
	{
		LOG(ERROR) << "Fail to seek file in pos(" << pos << ") |" << file_entry.path;
		return nullptr;
	}

	return file;
}
#endif

}
