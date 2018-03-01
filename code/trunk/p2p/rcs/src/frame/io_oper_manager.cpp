/*#############################################################################
 * �ļ���   : io_oper_manager.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��22��
 * �ļ����� : IoOperManager��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��: [in] session ����Session
 *         [in] torrent ����Torrent
 *         [in] torrrent_file Torrent�������ļ�����
 *         [in] resume_file FastResume�ļ�
 *         [in] io_thread IO���ȶ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
IoOperManager::IoOperManager(const TorrentSP& torrent, DiskIoThread& io_thread)
    : torrent_(torrent), io_thread_(io_thread)
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
IoOperManager::~IoOperManager()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��10��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool IoOperManager::Init()
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	std::string info_hash_str(torrent->info_hash()->to_string());
	
	torrent_path_ = torrent->save_path() / fs::path(info_hash_str);
	
	// ���torrentĿ¼�����ڣ����ȴ���Ŀ¼
	if (!fs::exists(torrent_path_))
	{
		if (!fs::create_directory(torrent_path_))
		{
			LOG(ERROR) << "Fail to create torrent directory | " << torrent_path_;
			return false;
		}
	}

	// ������Դ�ļ�IO����
	file_storage_.reset(new FileStorage(torrent));
	if (!file_storage_->Initialize())
	{
		LOG(ERROR) << "Fail to initialize file storage";
		return false;
	}

	// ��/����fast-resume�ļ�
	fs::path fast_resume_path = torrent_path_ / fs::path(info_hash_str + ".fastresume");
	fast_resume_file_.reset(new File(fast_resume_path, File::in | File::out));
	if (!fast_resume_file_->IsOpen())
	{
		LOG(ERROR) << "Fail to open fast-resume file | " << info_hash_str;
		return false;
	}

	// ��/����metadata�ļ�
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
 * ��  ��: ɾ�������ļ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void IoOperManager::RemoveAllFiles()
{
	// ɾ����Դ�ļ�
	if (file_storage_ && !file_storage_->Remove())
	{
		LOG(ERROR) << "Fail to remove resource file";
	}

	// ɾ��fast-resume�ļ�
	if (fast_resume_file_ && !fast_resume_file_->Remove())
	{
		LOG(ERROR) << "Fail to remove fast-resume file";
	}

	// ɾ��metadata�ļ�
	if (metadata_file_ && !metadata_file_->Remove())
	{
		LOG(ERROR) << "Fail to remove metadata file";
	}

	// ɾ�����Ŀ¼
	if (-1 == ::remove(torrent_path_.string().c_str()))
	{
		LOG(ERROR) << "Fail to remove torrent directory";
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �첽��
 * ��  ��: [in] request ��������
 *         [in] handler �ص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �첽д
 * ��  ��: [in] request ��������
 *         [in] buf ������
 *         [in] handler �ص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��ȡ����ʵ��
 * ��  ��: [in] buf ���ݻ�����
 *         [in] piece_index	piece����
 *         [in] offset Ƭ��ƫ��
 *         [in] size ��ȡ���ݵĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
int IoOperManager::ReadData(char* buf, int piece_index, int offset, int size)
{
	BC_ASSERT(buf);

    // ��ǰpiece_index��slot��һһ��Ӧ��

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
 * ��  ��: д����ʵ��
 * ��  ��: [in] buf	���ݻ�����
 *         [in] piece_index	piece����
 *         [in] offset Ƭ��ƫ��
 *         [in] size ��ȡ���ݵĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �첽��ȡfast-resume����
 * ��  ��: [in] handler �ص�����
 *         [in] buf ��������
 *         [out] len ��ȡ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncReadFastResume(IoJobHandler handler)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new ReadResumeJob(torrent->session().work_ios(), torrent, handler));

    io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �첽����fast-resume����
 * ��  ��: [in] handler �ص�����
 *         [in] buf ���ݻ�����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncWriteFastResume(IoJobHandler handler, const std::string& data)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new WriteResumeJob(torrent->session().work_ios(), torrent, handler, data));

    io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��fastresume����д���ļ�
 * ��  ��: [in] data д������
 * ����ֵ: д���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteFastResume(const std::string& data)
{
	return WriteFile(fast_resume_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡfastresume
 * ��  ��: [out] buf ��ȡfast-resume���ݵĻ�����
 * ����ֵ: ��ȡ�Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool IoOperManager::ReadFastResume(std::string& data)
{
	return ReadFile(fast_resume_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �첽��ȡmetadata����
 * ��  ��: [in] handler �ص�����
 *         [in] buf ��������
 *         [out] len ������������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncReadMetadata(IoJobHandler handler)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new ReadMetadataJob(torrent->session().work_ios(), torrent, handler));

	io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �첽��ȡmetadata����
 * ��  ��: [in] handler �ص�����
 *         [in] buf ��������
 *         [out] len ������������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void IoOperManager::AsyncWriteMetadata(IoJobHandler handler, const std::string& data)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new WriteMetadataJob(torrent->session().work_ios(), torrent, handler, data));

	io_thread_.AddJob(job);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��metadata����д���ļ�
 * ��  ��: [in] buf ����ָ��
 *         [in] len ���ݳ���
 * ����ֵ: д���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteMetadata(const std::string& data)
{
	return WriteFile(metadata_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡmetadata
 * ��  ��: [out] buf ��ȡmetadata���ݵĻ�����
 *         [in] len ���ݵĻ���������
 * ����ֵ: ��ȡ�Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool IoOperManager::ReadMetadata(std::string& data)
{
	return ReadFile(metadata_file_.get(), data);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡ����
 * ��  ��: [in] file_path �ļ�·��
 *         [out] data ���ݻ�����
 * ����ֵ: ��ȡ�Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��11��11��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���ļ���д������
 * ��  ��: [in] file_path �ļ�·��
 *         [in] data ���ݻ�����
 * ����ֵ: д���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��11��11��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteFile(File* file, const std::string& data)
{
	return WriteFileImpl(file, data.c_str(), data.size());
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡ����
 * ��  ��: [out] buf ��ȡmetadata���ݵĻ�����
 *         [in] len ���ݵĻ���������
 * ����ֵ: ��ȡ�Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ������д���ļ�
 * ��  ��: [in] buf ����ָ��
 *         [in] len ���ݳ���
 * ����ֵ: д���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool IoOperManager::WriteFileImpl(File* file, const char* buf, uint64 len)
{
	if(!file->IsOpen()) 
	{
		LOG(ERROR) << "File is not opened";
		return false;
	}

	error_code error;

	file->SetSize(0, error); //�����ļ���С
	if(error)
	{
		LOG(ERROR) << "Set file size error | " << error.message();
		return false;
	}

	file->Seek(0, File::begin, error); //�����ļ�ָ��
	if(error)
	{
		LOG(ERROR) << "Seek file error | " << error.message();
		return false;
	}

	uint64 bytes_written = file->Write(buf, len, error); //д������
	if(error || (bytes_written != len))
	{
		LOG(ERROR) << "Write file error | error: " << error.message() 
			       << " len: " << len << " read size: " << bytes_written;
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: �첽У��piece��info-hash
 * ��  ��: [in] handler �ص�����
 *         [in] piece piece����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��6��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void IoOperManager::AsyncVerifyPieceInfoHash(IoJobHandler handler, uint64 piece)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	DiskIoJobSP job(new VerifyPieceInfoHashJob(torrent->session().work_ios(), torrent, handler, piece));

	io_thread_.AddJob(job);
}

}
