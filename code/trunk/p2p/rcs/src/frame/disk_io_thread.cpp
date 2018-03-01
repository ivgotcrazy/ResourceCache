/*#############################################################################
 * �ļ���   : disk_io_thread.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : DiskIoThread/DiskBufferPoolʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <algorithm>
#include "disk_io_thread.hpp"
#include "disk_io_job.hpp"
#include "bc_assert.hpp"
#include "io_oper_manager.hpp"
#include "bc_util.hpp"
#include "mem_buffer_pool.hpp"
#include "torrent.hpp"
#include "session.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: [in] s Session
 *         [in] t Torrent
 *         [in] piece_index piece����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntry::CachedPieceEntry(Session& s, const TorrentSP& t, uint64 piece_index) 
	: cache_index(t.get(), piece_index)
	, session(s)
	, torrent(t)
    , piece_size(0)
	, num_blocks(0)
	, finished_blocks(0)
	, last_use(TimeNow())
	, verified(false)
{
	piece_size = t->GetPieceSize(piece_index);
	uint64 common_block_size = t->GetCommonBlockSize();
	num_blocks = CALC_FULL_MULT(piece_size, common_block_size);
	blocks.resize(num_blocks);

	MemBufferPool& mem_pool = session.mem_buf_pool();

	// ��ʼ��block�����С
	for (uint64 block_index = 0; block_index < num_blocks; ++block_index)
	{
		blocks[block_index].block_size = t->GetBlockSize(piece_index, block_index);
		blocks[block_index].buf = mem_pool.AllocDataBuffer(DATA_BUF_BLOCK);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntry::~CachedPieceEntry()
{
	MemBufferPool& mem_pool = session.mem_buf_pool();
	for(CachedBlockEntry& entry : blocks)
	{
		if (entry.buf)
		{
			mem_pool.FreeDataBuffer(DATA_BUF_BLOCK, entry.buf);
		}
	}
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: [in] pool �����
 *         [in] cache_blocks_limit ����block����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoThread::DiskIoThread(MemBufferPool& pool) 
	: mem_buf_pool_(pool)
	, read_cache_limit_(MAX_READ_CACHE_NUM)
	, write_cache_limit_(MAX_WRITE_CACHE_NUM)
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
DiskIoThread::~DiskIoThread()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���߳�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DiskIoThread::Start()
{
	LOG(INFO) << "Start disk IO thread...";
	boost::thread t(boost::bind(&DiskIoThread::ThreadFun, this));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���job
 * ��  ��: [in] job ����ӵ�Job
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DiskIoThread::AddJob(const DiskIoJobSP& job)
{
	boost::mutex::scoped_lock lock(job_mutex_);

	LOG(INFO) << "Disk IO job num | " << job_queue_.size();

	job_queue_.push(job);
	condition_.notify_all();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���߳����к���
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DiskIoThread::ThreadFun()
{
	while (true)
	{
		DiskIoJobSP job;
		{
			boost::mutex::scoped_lock lock(job_mutex_);
			while (job_queue_.empty())
			{
				condition_.wait(lock);
			}

			job = job_queue_.front();
			job_queue_.pop();
		}

		job->Do(*this);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����
 * ��  ��: [out] read_job ���ݶ�ȡJob
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool DiskIoThread::ReadData(ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	// ÿ�ζ�ȡ�����ݲ��ܳ���piece�߽�
	if (read_job->offset + read_job->len > torrent->GetPieceSize(read_job->piece))
	{
		LOG(ERROR) << "Invalid read data job | " << read_job;
		return false;
	}

	//TODO: ÿ�ζ�ȡ�����ݲ��ܳ���һ��block����Ҫ����ʵ��ȷ��
	if (read_job->len > torrent->GetCommonBlockSize())
	{
		LOG(ERROR) << "Read data size exceed block size | read_size: " << read_job->len 
			       << ", block_size:" << torrent->GetCommonBlockSize();
		return false;
	}

	// ���ڴ��������һ��block��С�Ļ���
	read_job->buf = mem_buf_pool_.AllocDataBuffer(DATA_BUF_BLOCK);
	BC_ASSERT(read_job->buf);

	// �ӻ����ж�ȡ����
	return TryReadFromCache(read_job);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ÿ�ζ����ݶ��ȳ��Դӻ����ж�ȡ�����������û���ٴӴ����ж�
 * ��  ��: [out] job ���ݶ�ȡJob
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool DiskIoThread::TryReadFromCache(ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	// ��ȡ����
	CachedPieceEntrySP piece_entry = GetReadCacheEntry(read_job);
	if (!piece_entry)
	{
		LOG(ERROR) << "Fail to read data from cache | " << *read_job;
		return false;
	}
	
	// �ӻ����ȡ����
	uint64 block_size = torrent->GetCommonBlockSize();
	uint64 read_block = read_job->offset / block_size;
	uint64 read_offset = read_job->offset % block_size;
	int left_bytes = read_job->len;
	int read_bytes = 0, write_offset = 0;
	while (left_bytes > 0)
	{
		// ��ȡ�����ݳ���block���ޣ���Ҫ��ȡ���block
		if (left_bytes + read_offset > block_size)
		{
			read_bytes = block_size - read_offset;
			left_bytes -= read_bytes;
		}
		else // ֻ���ȡһ��block����
		{
			read_bytes = left_bytes;
			left_bytes = 0;
		}

		// �ӻ�����job�п����ڴ�
		std::memcpy(read_job->buf + write_offset, 
			        piece_entry->blocks[read_block].buf + read_offset, 
					read_bytes);

		// ֻ�е�һ�ζ�ȡ����Ҫ����read_offset������϶����Ǵ�
		// block��ʼλ�ÿ�ʼ��ȡ
		read_offset = 0;
		read_block++;
		BC_ASSERT(read_block <= piece_entry->num_blocks);
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡpiece����
 * ��  ��: [in] job ������Job
 * ����ֵ: piece����
 * ��  ��:
 *   ʱ�� 2013��10��16��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntrySP DiskIoThread::GetReadCacheEntry(const ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	cache_status_.blocks_read++;

	CachedPieceEntrySP piece_entry;

	HashView& hash_view = read_cache_set_.get<0>();		
	CachedPieceSetIter iter = hash_view.find(CachedPieceIndex(torrent.get(), read_job->piece), 
		CachedPieceIndexHash(), CachedPieceIndexEqual());
	if (iter != read_cache_set_.end())
	{
		piece_entry = *iter;
		piece_entry->last_use = TimeNow();
		
		LOG(INFO) << "### Found one cached piece";

		cache_status_.blocks_read_hit++;
	}
	else
	{
		piece_entry = ReadPieceToCache(read_job);
		BC_ASSERT(piece_entry);

		read_cache_set_.insert(CachedPieceSet::value_type(piece_entry));

		LOG(INFO) << ">>> Insert one cached piece";

		cache_status_.reads++;
	}

	return piece_entry;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ӵ��̶���һ��piece������
 * ��  ��: [in] read_job ����������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntrySP DiskIoThread::ReadPieceToCache(const ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	// ��ȡ���û���ռ�
	CachedPieceEntrySP piece_entry = GetAvailableReadCachePiece(read_job);

	//TODO: ������������Ż�ʹ��readv��һ�ζ�ȡ������棬���ٴ���IO����
	char* read_buf = 0;
	uint64 read_size = 0, offset = 0, loop = 0;
	for ( ; loop < piece_entry->num_blocks; ++loop)
	{
		read_buf = piece_entry->blocks[loop].buf;
		read_size = piece_entry->blocks[loop].block_size;
		if (!torrent->io_oper_manager()->ReadData(read_buf, read_job->piece, offset, read_size))
		{
			break;
		}
		offset += read_size;
		BC_ASSERT(offset <= piece_entry->piece_size);
	}

	// ��ȡʧ�ܣ�����Ҫ�ͷ�����Ļ�����Դ
	if (loop != piece_entry->num_blocks)
	{
		LOG(ERROR) << "Fail to read piece to cache | piece: " << read_job->piece;

		// Ϊ�˱��⸴�ӵ�����Ԫ�ش�������ֱ�ӽ�entry��ʹ��ʱ������Ϊһ�������ʱ�䣬
		// �������´��ٲ��Ҷ������ʱ�򣬿����������ô˶�����
		piece_entry->last_use = ptime(boost::gregorian::date(1900, boost::gregorian::Jan, 1));

		return CachedPieceEntrySP();
	}

	return piece_entry;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ���ö�����ռ䲢���ݲ������г�ʼ��
 * ��  ��: [out] job ���ݶ�ȡJob
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntrySP DiskIoThread::GetAvailableReadCachePiece(const ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	BC_ASSERT(read_cache_set_.size() <= read_cache_limit_);

	// ��������治�����ˣ���ѡ�����δ���ʵ�entry
	if (read_cache_set_.size() == read_cache_limit_)
	{
		BC_ASSERT(!read_cache_set_.empty());
		LastUseView& last_use_view = read_cache_set_.get<1>();

		// �����ײ��ľ������δʹ�õĻ���
		last_use_view.erase(last_use_view.begin());
	}

	// ����һ���µĻ������
	CachedPieceEntrySP piece_entry(new CachedPieceEntry(torrent->session(), torrent, read_job->piece));
	read_cache_set_.insert(CachedPieceSet::value_type(piece_entry));

	return piece_entry;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����д����
 * ��  ��: [in] torrent ����Torrent
 *         [in] piece ��Ƭ����
 * ����ֵ: ���õ�CachedPieceEntry
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntrySP DiskIoThread::FindWriteCachePiece(const TorrentSP& torrent, uint64 piece)
{
	// ����д����
	CachedPieceMapIter iter = unfinished_write_cache_.find(CachedPieceIndex(torrent.get(), piece));
	if (iter != unfinished_write_cache_.end())
	{
		LOG(INFO) << "Success to find unfinished cache piece(" << piece << ")";
		return iter->second;
	}

	CachedPieceEntrySP piece_entry;

	// ����������
	if (unfinished_write_cache_.size() + finished_write_cache_.size() >= write_cache_limit_)
	{
		return piece_entry;
	}

	piece_entry.reset(new CachedPieceEntry(torrent->session(), torrent, piece));
	unfinished_write_cache_.insert(CachedPieceMap::value_type(
		CachedPieceIndex(torrent.get(), piece), piece_entry));

	LOG(INFO) << "New one write cache piece(" << piece << ")";

	return piece_entry;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ֱ��д��Ӳ��
 * ��  ��: [in] job WriteDataJob
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��06��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool DiskIoThread::WriteDataToDisk(const WriteDataJobSP& write_job)
{
	TorrentSP torrent = write_job->torrent;

	LOG(WARNING) << "Write data to disk directly | " << *write_job;

	int64 result = torrent->io_oper_manager()->WriteData(write_job->buf, 
		write_job->piece, write_job->offset, write_job->len);
	if (result <= 0 || static_cast<uint64>(result) != write_job->len)
	{
		LOG(ERROR) << "Fail to write data to disk | " << *write_job;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: д������
 * ��  ��: [in] write_job д��������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool DiskIoThread::WriteData(const WriteDataJobSP& write_job)
{
	TorrentSP torrent = write_job->torrent;

	cache_status_.blocks_write++;

	// ����д���棬���ʵ��û�п��õ�д���棬��ֱ��д��Ӳ��
	CachedPieceEntrySP piece_entry = FindWriteCachePiece(torrent, write_job->piece);
	if (!piece_entry)
	{
		cache_status_.write_to_disk_directly++;
		return WriteDataToDisk(write_job);
	}

	// ������д�뻺��
	uint64 block_size = torrent->GetCommonBlockSize();
	uint64 block_index = write_job->offset / block_size;
	BC_ASSERT(block_index < piece_entry->blocks.size());
	
	CachedBlockEntry& block_entry = piece_entry->blocks[block_index];

	uint64 block_offset = write_job->offset % block_size;

	//block_offset �� cursor����ȣ�����Ϊ���ظ��İ�
	if (block_offset != block_entry.cursor)
	{
		LOG(WARNING) << " Repeat Block data ";
		return true;
	}

	BC_ASSERT(block_offset == block_entry.cursor);
		
	// ������copy������
	std::memcpy(block_entry.buf + block_offset, write_job->buf, write_job->len);

	LOG(INFO) << "Success to write cached data | " << *write_job;

	// ˢ�»�����������
	block_entry.cursor += write_job->len;
	piece_entry->last_use = TimeNow();
	if (block_entry.cursor == block_entry.block_size)
	{
		block_entry.finished = true;
		piece_entry->finished_blocks++;

		LOG(INFO) << "One block of cache piece finished | [" 
			<< piece_entry->cache_index.piece << ":" << block_index << "]";

		if (piece_entry->finished_blocks == piece_entry->num_blocks)
		{
			unfinished_write_cache_.erase(CachedPieceIndex(torrent.get(), write_job->piece));
			finished_write_cache_.push_back(piece_entry);
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������piece����д�����
 * ��  ��: [in] piece_entry ����piece
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��06��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool DiskIoThread::WriteCachedPieceToDisk(const CachedPieceEntrySP& piece_entry)
{
	TorrentSP torrent = piece_entry->torrent.lock();
	if (!torrent) return false;

	cache_status_.writes++;

	uint64 block_size = 0, offset = 0, write_size = 0;

	for (uint64 i = 0; i < piece_entry->num_blocks; ++i)
	{
		BC_ASSERT(piece_entry->blocks[i].buf);
		block_size = piece_entry->blocks[i].block_size;

		write_size = torrent->io_oper_manager()->WriteData(piece_entry->blocks[i].buf, 
			piece_entry->cache_index.piece, offset, block_size);

		if (write_size != block_size)
		{
			LOG(ERROR) << "Fail to flush cached piece(" 
				       << piece_entry->cache_index.piece << ")";
			return false; // ����д���Ѿ�û��ʲô������
		}

		offset += block_size;
	}

	LOG(INFO) << "Success to flush cached piece(" << piece_entry->cache_index.piece << ")";

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ƥ���cache��ɾ��������flush���ԣ�ֻҪverify��cacheһ���ᱻflush��
 *         ���̣����ԣ�������finished�����cache��ֱ��ɾ������
 * ��  ��: [in] key ����torrent
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��11��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void DiskIoThread::ClearWriteCache(void* key)
{
	boost::mutex::scoped_lock lock(write_cache_mutex_);

	// ��������ɻ���
	for (CachedPieceListIter iter = finished_write_cache_.begin(); 
		 iter != finished_write_cache_.end(); )
	{
		if ((*iter)->cache_index.key == key)
		{
			iter = finished_write_cache_.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	// δ������ɻ���
	for (CachedPieceMapIter iter = unfinished_write_cache_.begin();
		 iter != unfinished_write_cache_.end(); )
	{
		if (iter->second->cache_index.key == key)
		{
			iter = unfinished_write_cache_.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: 
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool DiskIoThread::FlushWriteCache(void* key, uint64 piece)
{
	CachedPieceListIter iter = finished_write_cache_.begin();
	for ( ; iter != finished_write_cache_.end(); )
	{
		if ((*iter)->cache_index.key == key 
			&& (*iter)->cache_index.piece == piece)
		{
			BC_ASSERT((*iter)->verified);

			break;
		}		
	}

	if (iter == finished_write_cache_.end()) return false;

	bool result = WriteCachedPieceToDisk(*iter);

	// ����д�����Ƿ�ɹ��������������
	finished_write_cache_.erase(iter);

	return result;
}

/*------------------------------------------------------------------------------
 * ��  ��: У��piece��info-hash
 * ��  ��: [in] storage �洢����
 *         [in] piece piece����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��6��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool DiskIoThread::VerifyPieceInfoHash(const TorrentSP& torrent, uint64 piece)
{
	// ����ɶ����в�����ҪУ���piece����ǰ��flush������ֻҪ��һ��piece��ɣ�
	// �ͻᴴ��һ��flush jobȥflush���ݣ���ˣ���ɶ��еĳ��Ȳ���̫������
	// ��ʱ���ñ������ҵķ�ʽ�������ɶ��г��ȿ��ܺܳ�������Ҫ���Ǹ����㷨
	CachedPieceListIter iter = finished_write_cache_.begin();
	for ( ; iter != finished_write_cache_.end(); ++iter)
	{
		if ((*iter)->cache_index.key == torrent.get()
			&& (*iter)->cache_index.piece == piece)
		{
			break;
		}
	}

	BC_ASSERT(iter != finished_write_cache_.end());
	BC_ASSERT(!((*iter)->verified));

    uint64 piece_size = (*iter)->piece_size;
    char* piece_data = new char[piece_size];
    char* block_data = piece_data;

    const std::vector<CachedBlockEntry>& blocks = (*iter)->blocks;
    for (decltype(blocks.size()) i=0; i < blocks.size(); ++i)
    {
        memcpy(block_data, blocks[i].buf, blocks[i].block_size);
        block_data += blocks[i].block_size;
    }

	// У��piece
    bool correct = torrent->VerifyPiece(
		piece, piece_data, static_cast<uint32>(piece_size));  

    delete[] piece_data;

	// hashУ��ʧ�ܺ���Ҫ��piece����ɾ��
    if (!correct)
    {
		finished_write_cache_.erase(iter);
        return false;
    }
	
    (*iter)->verified = true;

	// ����У��ɹ�����Ҫ����������flush��Ӳ�̡���ΪУ�����
	// PiecePicker�Ὣ��piece���Ϊ�����أ��û��п������ϾͻῪ
	// ʼ�������piece������˴�ʹ���첽flush�����п��ܴӴ�����
	// �޷�������piece��Ϊ�˼򵥻������������������ʹ��ͬ��������
	// ����У�鶯����������첽�ģ���ˣ���ǰ��˵�����ֲ����ܹ�
	// ����ʹ��۱�֤����һ���ԡ�
	return FlushWriteCache(torrent.get(), piece);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ����ͳ������
 * ��  ��: 
 * ����ֵ: ͳ������
 * ��  ��:
 *   ʱ�� 2013��11��6��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
CacheStatus DiskIoThread::GetCacheStatus()
{
	cache_status_.read_cache_size = read_cache_set_.size();
	cache_status_.finished_write_cache_size = finished_write_cache_.size();
	cache_status_.unfinished_write_cache_size = unfinished_write_cache_.size();

	return cache_status_;
}

}
