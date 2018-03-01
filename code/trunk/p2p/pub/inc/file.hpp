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

#ifndef HEADER_FILE
#define HEADER_FILE

#include "depend.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{

namespace fs = boost::filesystem;

class File : public boost::noncopyable
{
public:

	// 文件读写定位模式
	class SeekMode
	{
	friend class File;
	private:
		SeekMode(int v): val_(v) {}
		int val_;
	};

	static const SeekMode begin;
	static const SeekMode end;

	// 文件打开模式
	class OpenMode
	{
	friend class File;
	public:
		OpenMode(): mask_(0) {}
		OpenMode operator|(OpenMode m) const
		{ return OpenMode(m.mask_ | mask_); }

		OpenMode operator&(OpenMode m) const
		{ return OpenMode(m.mask_ & mask_); }

		OpenMode operator|=(OpenMode m)
		{
			mask_ |= m.mask_;
			return *this;
		}

		bool operator==(OpenMode m) const { return mask_ == m.mask_; }
		bool operator!=(OpenMode m) const { return mask_ != m.mask_; }
		operator bool() const { return mask_ != 0; }

	private:

		OpenMode(int val): mask_(val) {}
		int mask_;
	};

	static const OpenMode in;
	static const OpenMode out;

	// File
	File();
	File(fs::path const& p, OpenMode m);
	~File();

	bool Open(fs::path const& p, OpenMode m);
	bool IsOpen() const;
	void Close();
	bool Remove();
	bool SetSize(uint64 size, error_code& ec);

	uint64 Write(const char*, uint64 num_bytes, error_code& ec);
	uint64 Read(char*, uint64 num_bytes, error_code& ec);

	uint64 Seek(uint64 pos, SeekMode m, error_code& ec);
	uint64 Tell(error_code& ec);
	
	const fs::path& file_path() const { return file_path_; }

    uint64 GetSize(error_code& error) const;

private:
	int			fd_;		// 文件描述符
	fs::path	file_path_;	// 文件全路径
};

}

#endif
