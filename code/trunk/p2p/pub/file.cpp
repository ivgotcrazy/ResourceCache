/*

Copyright (c) 2006, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "file.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_RANDOM
#define O_RANDOM 0
#endif

namespace
{
	enum { mode_in = 1, mode_out = 2 };

	int MapOpenMode(int m)
	{
		if (m == (mode_in | mode_out)) return O_RDWR | O_CREAT | O_BINARY | O_RANDOM;
		if (m == mode_out) return O_WRONLY | O_CREAT | O_BINARY | O_RANDOM;
		if (m == mode_in) return O_RDONLY | O_BINARY | O_RANDOM;

		return 0;
	}

}

namespace BroadCache
{

// 定义文件读写模式和文件定位模式
const File::OpenMode File::in(mode_in);
const File::OpenMode File::out(mode_out);
const File::SeekMode File::begin(SEEK_SET);
const File::SeekMode File::end(SEEK_END);

File::File() : fd_(-1) 
{
}

File::File(fs::path const& path, OpenMode mode) : fd_(-1)
{
	Open(path, mode);
}

File::~File()
{
	Close();
}

bool File::Open(fs::path const& path, OpenMode mode)
{
	Close();

	// rely on default umask to filter x and w permissions
	// for group and others
	int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	fd_ = ::open(path.c_str(), MapOpenMode(mode.mask_), permissions);
	if (fd_ == -1)
	{
		LOG(ERROR) << "Fail to open file: " << path.c_str();
		return false;
	}

	// update file path
	file_path_ = path;

	return true;
}

bool File::IsOpen() const 
{
	return fd_ != -1;
}

void File::Close()
{
	if (fd_ == -1) return;
	::close(fd_);
	fd_ = -1;
}

bool File::Remove()
{
	Close();

	if (-1 == ::remove(file_path_.string().c_str()))
	{
		return false;
	}

	return true;
}

uint64_t File::Read(char* buf, uint64 num_bytes, error_code& ec)
{
	int64 ret = ::read(fd_, buf, num_bytes);
	if (ret < 0) 
	{
		ec = error_code(errno, boost::system::get_generic_category());
	}

	return ret;
}

uint64 File::Write(const char* buf, uint64 num_bytes, error_code& ec)
{
	int32 ret = ::write(fd_, buf, num_bytes);
	if (ret == -1) 
	{
		ec = error_code(errno, boost::system::get_generic_category());
	}

	return static_cast<uint64>(ret);
}

bool File::SetSize(uint64 size, error_code& ec)
{
	if (ftruncate(fd_, size) < 0)
	{
		ec = error_code(errno, boost::system::get_generic_category());
		return false;
	}

	return true;
}

uint64 File::Seek(uint64 offset, SeekMode m, error_code& ec)
{
	int64 ret = lseek(fd_, offset, m.val_);
	if (ret < 0) 
	{
		ec = error_code(errno, boost::system::get_generic_category());
	}

	return static_cast<uint64>(ret);
}

uint64 File::Tell(error_code& ec)
{
	int64 ret = lseek(fd_, 0, SEEK_CUR);
	if (ret < 0) 
	{
		ec = error_code(errno, boost::system::get_generic_category());
	}

	return static_cast<uint64>(ret);
}

/**************************************************************************
 * 描  述: 获取文件的大小
 * 参  数: [out]error 返回的错误代码
 * 返回值: 文件大小
 * 修  改:
 *   时间 2013.09.16
 *   作者 rosan
 *   描述 创建
 *************************************************************************/
uint64 File::GetSize(error_code& error) const
{
    struct stat st;
    if(fstat(fd_, &st) < 0)
    {
        error = error_code(errno, boost::system::get_generic_category());
        return 0;
    }
    
    return static_cast<uint64>(st.st_size);
}

}
