/*#############################################################################
 * �ļ���   : io_oper_manager.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��22��
 * �ļ����� : IoOperManager������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ����: �������д���IO����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class IoOperManager : public boost::noncopyable
{
public:	
	IoOperManager(const TorrentSP& torrent, DiskIoThread& io_thread);
	~IoOperManager();

	bool Init();
	void RemoveAllFiles();

	// �첽��дtorrent��Դ����
	void AsyncRead(const PeerRequest& request, char* buf, IoJobHandler handler);
	void AsyncWrite(const PeerRequest& request, char* buf, IoJobHandler handler);

	// ͬ����дtorrent��Դ����
	int  ReadData(char* buf, int piece_index, int offset, int size);
	int  WriteData(char* buf, int piece_index, int offset, int size);

	// �첽��дfast-resume����
	void AsyncReadFastResume(IoJobHandler handler);
	void AsyncWriteFastResume(IoJobHandler handler, const std::string& data);
	
	// ͬ����дfast-resume����
	bool ReadFastResume(std::string& data);
	bool WriteFastResume(const std::string& data);

	// �첽��дmetadata����
	void AsyncReadMetadata(IoJobHandler handler);
	void AsyncWriteMetadata(IoJobHandler handler, const std::string& data);

	// ͬ����дmetadata����
	bool ReadMetadata(std::string& data);
	bool WriteMetadata(const std::string& data);

	// �첽У��piece��info-hash
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

	// ����fast-resume��metadata�ļ���д
	boost::scoped_ptr<File> fast_resume_file_;
	boost::scoped_ptr<File> metadata_file_;

	// ������Դ�ļ���д
	boost::scoped_ptr<FileStorage> file_storage_;

	fs::path torrent_path_;
};

}


#endif
