/*#############################################################################
 * �ļ���   : torrent_file.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��13��
 * �ļ����� : TorrentFileʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "torrent_file.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ:
 * ��  ��: 
 *   ʱ�� 2013��10��8��
 *   ���� teck_zhou
 *   ���� �½�
 ----------------------------------------------------------------------------*/
TorrentFile::TorrentFile()
	: total_size_(0)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��: 
 *   ʱ�� 2013��10��8��
 *   ���� teck_zhou
 *   ���� �½�
 ----------------------------------------------------------------------------*/
TorrentFile::~TorrentFile()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����ļ�
 * ��  ��: [in] e �ļ�entry
 * ����ֵ:
 * ��  ��: 
 *   ʱ�� 2013��10��8��
 *   ���� teck_zhou
 *   ���� �½�
 ----------------------------------------------------------------------------*/
void TorrentFile::AddFile(const FileEntry& e)
{
	AddFile(e.path, e.size);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ͨ���ļ�·�����ļ���С�����ļ�·��
 * ��  ��: [in] path file��·��
 *         [in] data file�Ĵ�С
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��10��8��
 *   ���� vicent_pan
 *   ���� 
 ----------------------------------------------------------------------------*/
void TorrentFile::AddFile(const fs::path& file, uint64 size)
{
	FileEntry entry;
	entry.size = size;
	entry.path = file;
	entry.offset = total_size_;

	files_.push_back(entry);

	total_size_ += size;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������Դ�ļ���С
 * ��  ��: [in] size �ļ���С
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��10��27��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void TorrentFile::SetResourceFileSize(uint64 size)
{
	BC_ASSERT(files_.size() == 1);

	files_.front().size = size;

	total_size_ = size;
}

}