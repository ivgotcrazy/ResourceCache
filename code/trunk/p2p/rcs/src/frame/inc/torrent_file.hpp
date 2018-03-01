/*#############################################################################
 * �ļ���   : torrent_file.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��13��
 * �ļ����� : TorrentFile����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_TORRENT_FILE
#define HEADER_TORRENT_FILE

#include <string>
#include <vector>
#include "bc_typedef.hpp"

namespace BroadCache
{

namespace fs = boost::filesystem;

struct FileEntry
{
	FileEntry() : offset(0), size(0) {}

	fs::path path;	// ��Դ�ļ�����·��		
	uint64 offset;	// һ��Torrentֻ��һ����Դ�ļ�����ֵ��ԶΪ0
	uint64 size;	// �ļ���С		
};

class TorrentFile
{
public:
	TorrentFile();
	~TorrentFile();

	void AddFile(const FileEntry& e);
	void AddFile(const fs::path& file, uint64 size);

	void SetResourceName(std::string name) { name_ = name; }
	void SetResourceFileSize(uint64 size);

	typedef std::vector<FileEntry>::const_iterator FileIter;
	typedef std::vector<FileEntry>::const_reverse_iterator RvrsFileIter;
	FileIter file_at_offset(uint64 offset) const { return Begin(); }
	FileIter Begin() const { return files_.begin(); }
	FileIter End() const { return files_.end(); }
	RvrsFileIter Rbegin() const { return files_.rbegin(); }
	RvrsFileIter Rend() const { return files_.rend(); }

private:
	uint64 total_size_;				// �����ļ��ܴ�С
	std::vector<FileEntry> files_;	// Torrent�������ļ��б�
	std::string name_;				// ��Դ����
};

}

#endif