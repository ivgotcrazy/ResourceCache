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

#include "file_pool.hpp"

namespace BroadCache
{

using namespace boost::posix_time;
using boost::multi_index::nth_index;
using boost::multi_index::get;

File* FilePool::OpenFile(void* torrent, fs::path const& path, File::OpenMode mode)
{
	boost::mutex::scoped_lock lock(mutex_);

	// ͨ��path���Ҵ��ļ��Ƿ��Ѿ���
	typedef nth_index<FileSet, 0>::type PathView;
	PathView& pv = get<0>(files_);
	PathView::iterator iter = pv.find(path);

	// �ļ��Ѿ�������
	if (iter != pv.end())
	{
		LruFileEntry entry = *iter;
		entry.last_use = boost::posix_time::microsec_clock::universal_time();

		// ˵����������torrent����ʹ�ô��ļ�����������һ����Ҫִ��д��������϶��ǲ�����ģ�
		// ��Ȼ��������߶��Ƕ������ǿ��Ե�
		if (entry.key != torrent 
			&& (entry.mode != File::in || mode != File::out || mode != (File::in | File::out)))
		{
			return nullptr;
		}

		// ���ˣ�����ʲô����������µ�ǰ�ļ���ʹ����
		entry.key = torrent;

		// ����´򿪵�ģʽ���Ѵ�ģʽ�ĳ���������Ҫ���°�����ģʽ��
		if ((entry.mode & mode) != mode)
		{
			iter->file_ptr = nullptr;
			entry.file_ptr->Close();
			if (!entry.file_ptr->Open(path, mode))
			{
				files_.erase(iter);
				return nullptr;
			}

			entry.mode = mode;	// ˢ�´�ģʽ
		}

		// ˢ��entry
		pv.replace(iter, entry);

		return entry.file_ptr;
	}

	// �ļ�û�л���

	// �Ƴ����δʹ�õ��ļ�
	if ((int)files_.size() >= size_)
	{
		RemoveOldest();
	}

	// �����µ��ļ�����
	LruFileEntry entry;
	entry.file_ptr = new (std::nothrow)File;
	if (!entry.file_ptr)
	{
		return entry.file_ptr;
	}
	if (!entry.file_ptr->Open(path, mode))
	{
		return nullptr;
	}

	entry.mode = mode;
	entry.key  = torrent;
	entry.file_path = path;
	pv.insert(entry);

	return entry.file_ptr;
}

void FilePool::Release(void* storage)
{
	boost::mutex::scoped_lock l(mutex_);

	typedef nth_index<FileSet, 2>::type KeyView;
	KeyView& kv = get<2>(files_);

	KeyView::iterator start, end;
	boost::tie(start, end) = kv.equal_range(storage);
	kv.erase(start, end);
}

void FilePool::Release(fs::path const& path)
{
	boost::mutex::scoped_lock l(mutex_);

	typedef nth_index<FileSet, 0>::type PathView;
	PathView& pv = get<0>(files_);
	PathView::iterator iter = pv.find(path);

	// �ҵ��ļ��Ժ�ֻ�ǽ����������ɾ��������ȥ�ر��ļ�
	if (iter != pv.end())
	{
		pv.erase(iter);
	}
}

void FilePool::Resize(int size)
{
}

void FilePool::RemoveOldest()
{
	typedef nth_index<FileSet, 1>::type LruView;
	LruView& lv = get<1>(files_);
	LruView::iterator iter = lv.begin();

	while (int(files_.size()) >= size_)
	{
		lv.erase(iter++);
	}
}

}
