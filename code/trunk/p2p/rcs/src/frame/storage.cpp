/*#############################################################################
 * �ļ���   : storage.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : Storage���ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��: [in] files �������ļ�
           [in] path �ļ������·��
		   [in] fp �ļ��أ���Sessionͳһ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
FileStorage::FileStorage(const TorrentSP& t) : torrent_(t)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
FileStorage::~FileStorage()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FileStorage::Initialize()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	std::string info_hash_str(torrent->info_hash()->to_string());

	fs::path torrent_file = torrent->save_path() / fs::path(info_hash_str) / fs::path(info_hash_str);

	// ��/������Դ�ļ�
	torrent_file_.reset(new File(torrent_file, File::in | File::out));
	if (!torrent_file_->IsOpen())
	{
		LOG(ERROR) << "Fail to open torrent file | " << info_hash_str;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ���ȡһ������
 * ��  ��: [in] buf	    ������
 *         [in] slot	��λ����ǰ������Ϊ����piece
 *         [in] offset	�����slot��ƫ�ƣ���Ƭ��ƫ��
 *         [in] size	��ȡ�����ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FileStorage::Read(char* buf, uint64 slot, uint64 offset, uint64 size)
{
	return ReadWriteSingleFileImpl(buf, slot, offset, size, &File::Read);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ�д��һ�����ݣ���Ҫ���ǿ�piece�Ϳ��ļ���д��
 * ��  ��: [in] buf	��д�������
 *         [in] slot ��λ����ǰ������Ϊ����piece
 *   	   [in] offset �����slot��ƫ�ƣ���Ƭ��ƫ��
 *  	   [in] size д������ݳ���
 * ����ֵ: -1	����
 *         ����	д������ݴ�С
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FileStorage::Write(const char* buf, uint64 slot, uint64 offset, uint64 size)
{
	return ReadWriteSingleFileImpl(const_cast<char*>(buf), slot, offset, size, &File::Write);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ���д�Ĺ�����������
 * ��  ��: [in] buf		����������
 *         [in] slot	��λ����ǰ������Ϊ����piece
 *   	   [in] offset	�����slot��ƫ�ƣ���Ƭ��ƫ��
 *  	   [in] size	���ݳ���
 *         [in] oper	�����IO����
 * ����ֵ: -1	����
 *         ����	�ɹ�
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FileStorage::ReadWriteSingleFileImpl(char* buf, uint64 slot, uint64 offset, 
	                                      uint64 size, StorageIoOper oper)
{
	BC_ASSERT(buf);
	BC_ASSERT(torrent_file_);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	// Ĭ��д������ݲ��ܳ���piece�߽��
	if (offset + size > torrent->GetPieceSize(slot))
	{
		LOG(ERROR) << "Read/Write data exceed piece boundary |" << " piece: " 
			<< slot << " offset: " << offset << " write: " << size;
		return false;
	}

	uint64 file_offset = slot * torrent->GetCommonPieceSize() + offset;

	// �ļ�������Ҫ�������б���
	boost::mutex::scoped_lock lock(file_mutex_);

	error_code ec;

	// ��λ�ļ��ڲ���λ��
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
 * ��  ��: �Ƴ��ļ�
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FileStorage::Remove()
{
	boost::mutex::scoped_lock lock(file_mutex_);

	return torrent_file_->Remove();
}

#if 0
/*-----------------------------------------------------------------------------
 * ��  ��: �ļ���д�Ĺ�����������
 * ��  ��: [in] buf		����������
 *         [in] slot	��λ����ǰ������Ϊ����piece
 *   	   [in] offset	�����slot��ƫ�ƣ���Ƭ��ƫ��
 *  	   [in] size	���ݳ���
 *         [in] oper	�����IO����
 * ����ֵ: -1	����
 *         ����	�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FileStorage::ReadWriteImpl(char* buf, uint64 slot, uint64 offset, 
	                            uint64 size, StorageIoOper oper)
{
	BC_ASSERT(buf);

	uint64 file_offset = 0; // �ҵ��ļ����ļ���ƫ��
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

	// Ĭ��д������ݲ��ܳ���piece�߽��
	if (offset + size > slot_size)
	{
		// left_bytes = slot_size - offset;
		LOG(ERROR) << "Read/Write data exceed piece boundary |"
			       << " piece: " << slot
				   << " offset: " << offset
				   << " write: " << left_bytes;
		return false;
	}

	// �ļ�������Ҫ�������б���
	boost::mutex::scoped_lock lock(file_mutex_);

	uint64 buf_pos = 0;
	while (left_bytes > 0)
	{
		uint64 rw_bytes = left_bytes;

		// �ж��Ƿ���п��ļ����������������Ҫ��������ļ�
		if (file_offset + rw_bytes > file_iter->size)
		{
			BC_ASSERT(file_iter->size >= file_offset);
			rw_bytes = file_iter->size - file_offset;
		}

		// ��д�ļ�
		if (rw_bytes > 0)
		{
			error_code ec;

			// ��λ�ļ��ڲ���λ��
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

		// �����������û��д�룬��Ӧ��д����һ���ļ�
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
 * ��  ��: �ļ���д�Ĺ�����������
 * ��  ��: [in] slot ��λ����ǰ������Ϊ����piece
 *   	   [in] offset	�����slot��ƫ�ƣ���Ƭ��ƫ��
 *  	   [out] file_offset piece��ƫ��ת��Ϊ�ҵ��ļ����ļ���ƫ��
 * ����ֵ: -1	����
 *         ����	�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
FileStorage::FileEntryConstIter 
FileStorage::FindFileEntry(uint64 slot, uint64 offset, uint64* file_offset)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return files_.End();

	// �����������Դ��ƫ��
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
 * ��  ��: �ļ���д�Ĺ�����������
 * ��  ��: [in] file_entry �ļ�
 *         [in] offset �򿪺�λ
 * ����ֵ: -1	����
 *         ����	�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
File* FileStorage::OpenFile(const FileEntry& file_entry, uint64 offset)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return nullptr;

	// ���ļ�
	File* file = file_pool_.OpenFile(torrent.get(), file_entry.path, File::out | File::in);
	if (!file)
	{
		LOG(ERROR) << "Fail to open file | " << file_entry.path;
		return nullptr;
	}

	// ��λ��д��λ��
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
