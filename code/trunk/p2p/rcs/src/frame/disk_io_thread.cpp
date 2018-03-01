/*#############################################################################
 * 文件名   : disk_io_thread.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : DiskIoThread/DiskBufferPool实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描  述: 构造函数
 * 参  数: [in] s Session
 *         [in] t Torrent
 *         [in] piece_index piece索引
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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

	// 初始化block缓存大小
	for (uint64 block_index = 0; block_index < num_blocks; ++block_index)
	{
		blocks[block_index].block_size = t->GetBlockSize(piece_index, block_index);
		blocks[block_index].buf = mem_pool.AllocDataBuffer(DATA_BUF_BLOCK);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 构造函数
 * 参  数: [in] pool 缓存池
 *         [in] cache_blocks_limit 缓存block上限
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DiskIoThread::DiskIoThread(MemBufferPool& pool) 
	: mem_buf_pool_(pool)
	, read_cache_limit_(MAX_READ_CACHE_NUM)
	, write_cache_limit_(MAX_WRITE_CACHE_NUM)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DiskIoThread::~DiskIoThread()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 主线程启动
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskIoThread::Start()
{
	LOG(INFO) << "Start disk IO thread...";
	boost::thread t(boost::bind(&DiskIoThread::ThreadFun, this));
}

/*-----------------------------------------------------------------------------
 * 描  述: 添加job
 * 参  数: [in] job 待添加的Job
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskIoThread::AddJob(const DiskIoJobSP& job)
{
	boost::mutex::scoped_lock lock(job_mutex_);

	LOG(INFO) << "Disk IO job num | " << job_queue_.size();

	job_queue_.push(job);
	condition_.notify_all();
}

/*-----------------------------------------------------------------------------
 * 描  述: 主线程运行函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 读取数据
 * 参  数: [out] read_job 数据读取Job
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool DiskIoThread::ReadData(ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	// 每次读取的数据不能超过piece边界
	if (read_job->offset + read_job->len > torrent->GetPieceSize(read_job->piece))
	{
		LOG(ERROR) << "Invalid read data job | " << read_job;
		return false;
	}

	//TODO: 每次读取的数据不能超过一个block，还要经过实测确定
	if (read_job->len > torrent->GetCommonBlockSize())
	{
		LOG(ERROR) << "Read data size exceed block size | read_size: " << read_job->len 
			       << ", block_size:" << torrent->GetCommonBlockSize();
		return false;
	}

	// 从内存池中申请一个block大小的缓存
	read_job->buf = mem_buf_pool_.AllocDataBuffer(DATA_BUF_BLOCK);
	BC_ASSERT(read_job->buf);

	// 从缓存中读取数据
	return TryReadFromCache(read_job);
}

/*-----------------------------------------------------------------------------
 * 描  述: 每次读数据都先尝试从缓存中读取，如果缓存中没有再从磁盘中读
 * 参  数: [out] job 数据读取Job
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool DiskIoThread::TryReadFromCache(ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	// 获取缓存
	CachedPieceEntrySP piece_entry = GetReadCacheEntry(read_job);
	if (!piece_entry)
	{
		LOG(ERROR) << "Fail to read data from cache | " << *read_job;
		return false;
	}
	
	// 从缓存读取数据
	uint64 block_size = torrent->GetCommonBlockSize();
	uint64 read_block = read_job->offset / block_size;
	uint64 read_offset = read_job->offset % block_size;
	int left_bytes = read_job->len;
	int read_bytes = 0, write_offset = 0;
	while (left_bytes > 0)
	{
		// 读取的数据超过block界限，即要读取多个block
		if (left_bytes + read_offset > block_size)
		{
			read_bytes = block_size - read_offset;
			left_bytes -= read_bytes;
		}
		else // 只需读取一个block即可
		{
			read_bytes = left_bytes;
			left_bytes = 0;
		}

		// 从缓存向job中拷贝内存
		std::memcpy(read_job->buf + write_offset, 
			        piece_entry->blocks[read_block].buf + read_offset, 
					read_bytes);

		// 只有第一次读取才需要考虑read_offset，后面肯定都是从
		// block起始位置开始读取
		read_offset = 0;
		read_block++;
		BC_ASSERT(read_block <= piece_entry->num_blocks);
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取piece缓存
 * 参  数: [in] job 读数据Job
 * 返回值: piece缓存
 * 修  改:
 *   时间 2013年10月16日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 从磁盘读入一个piece到缓存
 * 参  数: [in] read_job 读数据任务
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntrySP DiskIoThread::ReadPieceToCache(const ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	// 获取可用缓存空间
	CachedPieceEntrySP piece_entry = GetAvailableReadCachePiece(read_job);

	//TODO: 后续这里可以优化使用readv，一次读取多个缓存，减少磁盘IO操作
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

	// 读取失败，则需要释放申请的缓存资源
	if (loop != piece_entry->num_blocks)
	{
		LOG(ERROR) << "Fail to read piece to cache | piece: " << read_job->piece;

		// 为了避免复杂的容器元素处理，这里直接将entry的使用时间设置为一个很早的时间，
		// 这样，下次再查找读缓存的时候，可以立即重用此读缓存
		piece_entry->last_use = ptime(boost::gregorian::date(1900, boost::gregorian::Jan, 1));

		return CachedPieceEntrySP();
	}

	return piece_entry;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取可用读缓存空间并根据参数进行初始化
 * 参  数: [out] job 数据读取Job
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntrySP DiskIoThread::GetAvailableReadCachePiece(const ReadDataJobSP& read_job)
{
	TorrentSP torrent = read_job->torrent;

	BC_ASSERT(read_cache_set_.size() <= read_cache_limit_);

	// 如果读缓存不够用了，则选用最久未访问的entry
	if (read_cache_set_.size() == read_cache_limit_)
	{
		BC_ASSERT(!read_cache_set_.empty());
		LastUseView& last_use_view = read_cache_set_.get<1>();

		// 排在首部的就是最久未使用的缓存
		last_use_view.erase(last_use_view.begin());
	}

	// 创建一块新的缓存插入
	CachedPieceEntrySP piece_entry(new CachedPieceEntry(torrent->session(), torrent, read_job->piece));
	read_cache_set_.insert(CachedPieceSet::value_type(piece_entry));

	return piece_entry;
}

/*-----------------------------------------------------------------------------
 * 描  述: 查找写缓存
 * 参  数: [in] torrent 所属Torrent
 *         [in] piece 分片索引
 * 返回值: 可用的CachedPieceEntry
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
DiskIoThread::CachedPieceEntrySP DiskIoThread::FindWriteCachePiece(const TorrentSP& torrent, uint64 piece)
{
	// 查找写缓存
	CachedPieceMapIter iter = unfinished_write_cache_.find(CachedPieceIndex(torrent.get(), piece));
	if (iter != unfinished_write_cache_.end())
	{
		LOG(INFO) << "Success to find unfinished cache piece(" << piece << ")";
		return iter->second;
	}

	CachedPieceEntrySP piece_entry;

	// 缓存数限制
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
 * 描  述: 将数据直接写入硬盘
 * 参  数: [in] job WriteDataJob
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月06日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 写入数据
 * 参  数: [in] write_job 写数据任务
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool DiskIoThread::WriteData(const WriteDataJobSP& write_job)
{
	TorrentSP torrent = write_job->torrent;

	cache_status_.blocks_write++;

	// 查找写缓存，如果实在没有可用的写缓存，则直接写入硬盘
	CachedPieceEntrySP piece_entry = FindWriteCachePiece(torrent, write_job->piece);
	if (!piece_entry)
	{
		cache_status_.write_to_disk_directly++;
		return WriteDataToDisk(write_job);
	}

	// 将数据写入缓存
	uint64 block_size = torrent->GetCommonBlockSize();
	uint64 block_index = write_job->offset / block_size;
	BC_ASSERT(block_index < piece_entry->blocks.size());
	
	CachedBlockEntry& block_entry = piece_entry->blocks[block_index];

	uint64 block_offset = write_job->offset % block_size;

	//block_offset 和 cursor不相等，就认为是重复的包
	if (block_offset != block_entry.cursor)
	{
		LOG(WARNING) << " Repeat Block data ";
		return true;
	}

	BC_ASSERT(block_offset == block_entry.cursor);
		
	// 将数据copy至缓存
	std::memcpy(block_entry.buf + block_offset, write_job->buf, write_job->len);

	LOG(INFO) << "Success to write cached data | " << *write_job;

	// 刷新缓存其他属性
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
 * 描  述: 将缓存的piece数据写入磁盘
 * 参  数: [in] piece_entry 缓存piece
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月06日
 *   作者 teck_zhou
 *   描述 创建
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
			return false; // 继续写入已经没有什么意义了
		}

		offset += block_size;
	}

	LOG(INFO) << "Success to flush cached piece(" << piece_entry->cache_index.piece << ")";

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 将匹配的cache都删除，根据flush策略，只要verify，cache一定会被flush到
 *         磁盘，所以，保存在finished里面的cache，直接删除即可
 * 参  数: [in] key 所属torrent
 * 返回值: 
 * 修  改:
 *   时间 2013年11月11日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskIoThread::ClearWriteCache(void* key)
{
	boost::mutex::scoped_lock lock(write_cache_mutex_);

	// 已下载完成缓存
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

	// 未下载完成缓存
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
 * 描  述: 
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
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

	// 不管写数据是否成功，都将缓存清除
	finished_write_cache_.erase(iter);

	return result;
}

/*------------------------------------------------------------------------------
 * 描  述: 校验piece的info-hash
 * 参  数: [in] storage 存储对象
 *         [in] piece piece索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月6日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool DiskIoThread::VerifyPieceInfoHash(const TorrentSP& torrent, uint64 piece)
{
	// 从完成队列中查找需要校验的piece，当前的flush策略是只要有一个piece完成，
	// 就会创建一个flush job去flush数据，因此，完成队列的长度不会太大，这里
	// 暂时先用遍历查找的方式，如果完成队列长度可能很长，则需要考虑更优算法
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

	// 校验piece
    bool correct = torrent->VerifyPiece(
		piece, piece_data, static_cast<uint32>(piece_size));  

    delete[] piece_data;

	// hash校验失败后需要将piece数据删除
    if (!correct)
    {
		finished_write_cache_.erase(iter);
        return false;
    }
	
    (*iter)->verified = true;

	// 数据校验成功后，需要立即将数据flush到硬盘。因为校验完后，
	// PiecePicker会将此piece标记为已下载，用户有可能马上就会开
	// 始下载这个piece，如果此处使用异步flush，则有可能从磁盘中
	// 无法读到此piece，为了简单化处理这种情况，这里使用同步操作，
	// 由于校验动作本身就是异步的，因此，当前来说，这种策略能够
	// 以最低代价保证数据一致性。
	return FlushWriteCache(torrent.get(), piece);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取缓存统计数据
 * 参  数: 
 * 返回值: 统计数据
 * 修  改:
 *   时间 2013年11月6日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
CacheStatus DiskIoThread::GetCacheStatus()
{
	cache_status_.read_cache_size = read_cache_set_.size();
	cache_status_.finished_write_cache_size = finished_write_cache_.size();
	cache_status_.unfinished_write_cache_size = unfinished_write_cache_.size();

	return cache_status_;
}

}
