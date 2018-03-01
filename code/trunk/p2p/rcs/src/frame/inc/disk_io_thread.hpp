/*#############################################################################
 * 文件名   : disk_io_thread.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : DiskIoThread/DiskBufferPool声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEAER_DISK_IO_THREAD
#define HEAER_DISK_IO_THREAD

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "bc_util.hpp"


namespace BroadCache
{

class PieceManager;
class DiskIoJob;
class ReadDataJob;
class WriteDataJob;
class MemPool;
class MemBufferPool;

struct CacheStatus
{
	CacheStatus()
		: reads(0)
		, writes(0)
		, blocks_read(0)
		, blocks_write(0)
		, blocks_read_hit(0)
		, write_to_disk_directly(0)
		, read_cache_size(0)
		, finished_write_cache_size(0)
		, unfinished_write_cache_size(0)
	{
	}

	uint64 reads;			// 读取操作的次数
	uint64 writes;			// 写入操作的次数

	uint64 blocks_read;		// 读取的block数
	uint64 blocks_write;	// 写入block数
	
	uint64 blocks_read_hit;	// 缓存读取命中block数

	uint64 write_to_disk_directly;	// 写缓存空间不够导致的直接写入硬盘次数

	uint64 read_cache_size;
	uint64 finished_write_cache_size;
	uint64 unfinished_write_cache_size;

	// 写入缓存命中率：(blocks_written - writes) / blocks_written
	// 读取缓存命中率：blocks_read_hit / blocks_read
};

typedef std::vector<CacheStatus> CacheStatusVec;

using namespace boost::multi_index;

/******************************************************************************
 * 描述：IO操作调度及应用层IO缓存
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
class DiskIoThread : public boost::noncopyable
{
public:
	DiskIoThread(MemBufferPool& pool);
	~DiskIoThread();

	void Start();

	void AddJob(const DiskIoJobSP& job);

	bool ReadData(ReadDataJobSP& read_job);
	bool WriteData(const WriteDataJobSP& write_job);

	bool VerifyPieceInfoHash(const TorrentSP& torrent, uint64 piece);
	
	void ClearWriteCache(void* key);

	CacheStatus GetCacheStatus();

private:

	struct CachedBlockEntry
	{
		CachedBlockEntry() : buf(0), block_size(0), cursor(0), finished(false) {}
		char* buf;
		uint64 block_size;
		uint64 cursor;
		bool finished;
	};

	struct CachedPieceIndex
	{
		CachedPieceIndex(void* k, uint64 p) : key(k), piece(p) {}

		bool operator==(const CachedPieceIndex& index) const
		{
			return (index.key == key) && (index.piece == piece); 
		}
		
		void* key;		// 这里使用torrent指针
		uint64 piece;	// piece索引
	};

	struct CachedPieceEntry : public boost::noncopyable
	{
		CachedPieceEntry(Session& s, const TorrentSP& t, uint64 piece_index);
		~CachedPieceEntry();

		bool operator==(const CachedPieceEntry& entry) const
		{
			return entry.cache_index == cache_index;
		}

		CachedPieceIndex cache_index;

		Session& session;		// 所属session
		TorrentWP torrent;		// 所属torrent

		uint64 piece_size;		// piece大小
		uint64 num_blocks;		// piece包含的block数
		uint64 finished_blocks;	// 完整block数量
		ptime last_use;			// 最近一次读/写时间
		bool verified;			// 是否已经做过hash校验

		std::vector<CachedBlockEntry> blocks;
	};	

	struct CachedPieceIndexHash
	{
		std::size_t operator()(const CachedPieceIndex& index) const
		{
			std::size_t seed = 0;
			boost::hash_combine(seed, index.key);
			boost::hash_combine(seed, index.piece);
			return seed;
		}
	};

	struct CachedPieceIndexEqual
	{
		bool operator()(const CachedPieceIndex& lhs, const CachedPieceIndex& rhs) const
		{
			return (lhs.key == rhs.key) && (lhs.piece == rhs.piece);
		}
	};

	typedef boost::shared_ptr<CachedPieceEntry> CachedPieceEntrySP;

	// cache map
	typedef boost::unordered_map
		<
			CachedPieceIndex, 
		    CachedPieceEntrySP, 
		    CachedPieceIndexHash,
			CachedPieceIndexEqual
		> CachedPieceMap;
	typedef CachedPieceMap::iterator CachedPieceMapIter;

	// cache list
	typedef std::list<CachedPieceEntrySP> CachedPieceList;
	typedef CachedPieceList::iterator CachedPieceListIter;

	// cache set
	typedef multi_index_container<
		CachedPieceEntrySP, 
		indexed_by<
			hashed_unique<
				member<CachedPieceEntry, CachedPieceIndex, &CachedPieceEntry::cache_index>,
				CachedPieceIndexHash
			>,
			ordered_non_unique<
				member<CachedPieceEntry, ptime, &CachedPieceEntry::last_use> 
			>
		> 
	>CachedPieceSet;
	typedef CachedPieceSet::iterator CachedPieceSetIter;
	typedef CachedPieceSet::nth_index<0>::type HashView;
	typedef CachedPieceSet::nth_index<1>::type LastUseView;

private:
	void ThreadFun();
	void CacheWriteBlock(const WriteDataJob& job);
	bool TryReadFromCache(ReadDataJobSP& read_job);
	CachedPieceEntrySP ReadPieceToCache(const ReadDataJobSP& read_job);
	CachedPieceEntrySP GetAvailableReadCachePiece(const ReadDataJobSP& read_job);
	CachedPieceEntrySP GetReadCacheEntry(const ReadDataJobSP& read_job);
	CachedPieceEntrySP FindWriteCachePiece(const TorrentSP& torrent, uint64 piece);
	bool WriteDataToDisk(const WriteDataJobSP& write_job);
	bool WriteCachedPieceToDisk(const CachedPieceEntrySP& piece_entry);
	bool FlushWriteCache(void* key, uint64 piece);

private:
	std::queue<DiskIoJobSP>		job_queue_;
	boost::mutex				job_mutex_;
	boost::condition_variable	condition_;
	
	MemBufferPool& mem_buf_pool_;

	// 读缓存通过缓存从磁盘读取的数据，来减少读磁盘次数
	CachedPieceSet read_cache_set_;
	boost::mutex read_cache_mutex_;

	// 写缓存通过缓存多次磁盘写入操作，来减少写磁盘次数
	CachedPieceMap unfinished_write_cache_; // 未下载完成的piece缓存
	CachedPieceList finished_write_cache_;  // 已下载完成的piece缓存
	boost::mutex write_cache_mutex_;

	// 缓存相关统计
	CacheStatus	cache_status_;

	// 读写缓存配额
	uint64 read_cache_limit_;
	uint64 write_cache_limit_;
};

}

#endif