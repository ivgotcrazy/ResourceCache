/*#############################################################################
 * 文件名   : torrent_file.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月13日
 * 文件描述 : TorrentFile声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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

	fs::path path;	// 资源文件完整路径		
	uint64 offset;	// 一个Torrent只有一个资源文件，此值永远为0
	uint64 size;	// 文件大小		
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
	uint64 total_size_;				// 所有文件总大小
	std::vector<FileEntry> files_;	// Torrent包含的文件列表
	std::string name_;				// 资源名称
};

}

#endif