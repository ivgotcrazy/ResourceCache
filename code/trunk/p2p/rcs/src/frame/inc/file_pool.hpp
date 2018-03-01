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

#ifndef HEADER_FILE_POOL
#define HEADER_FILE_POOL

#include "bc_util.hpp"
#include "depend.hpp"
#include "file.hpp"
#include "bc_typedef.hpp"


namespace BroadCache
{

using namespace boost::multi_index;

class FilePool : boost::noncopyable
{
public:
	FilePool(int size = 40) : size_(size) {}

	File* OpenFile(void* st, fs::path const& p, File::OpenMode m); 
	void  Release(void* st);
	void  Release(fs::path const& p);
	void  Resize(int size);

private:
	void RemoveOldest();

private:
	int size_;

	boost::mutex mutex_;

	struct LruFileEntry
	{
		LruFileEntry() : last_use(TimeNow()) {}
		mutable File* file_ptr;
		fs::path file_path;
		void* key;
		ptime last_use; 
		File::OpenMode mode;
	};

	typedef multi_index_container
			<
				LruFileEntry, 
				indexed_by
				<
					ordered_unique<member<LruFileEntry, fs::path, &LruFileEntry::file_path> >, 
				    ordered_non_unique<member<LruFileEntry, ptime, &LruFileEntry::last_use> >, 
				    ordered_non_unique<member<LruFileEntry, void*, &LruFileEntry::key> > 
				> 
			>FileSet;

	FileSet files_;

	
};

}

#endif