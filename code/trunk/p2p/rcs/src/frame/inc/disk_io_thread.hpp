/*#############################################################################
 * �ļ���   : disk_io_thread.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : DiskIoThread/DiskBufferPool����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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

	uint64 reads;			// ��ȡ�����Ĵ���
	uint64 writes;			// д������Ĵ���

	uint64 blocks_read;		// ��ȡ��block��
	uint64 blocks_write;	// д��block��
	
	uint64 blocks_read_hit;	// �����ȡ����block��

	uint64 write_to_disk_directly;	// д����ռ䲻�����µ�ֱ��д��Ӳ�̴���

	uint64 read_cache_size;
	uint64 finished_write_cache_size;
	uint64 unfinished_write_cache_size;

	// д�뻺�������ʣ�(blocks_written - writes) / blocks_written
	// ��ȡ���������ʣ�blocks_read_hit / blocks_read
};

typedef std::vector<CacheStatus> CacheStatusVec;

using namespace boost::multi_index;

/******************************************************************************
 * ������IO�������ȼ�Ӧ�ò�IO����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
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
		
		void* key;		// ����ʹ��torrentָ��
		uint64 piece;	// piece����
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

		Session& session;		// ����session
		TorrentWP torrent;		// ����torrent

		uint64 piece_size;		// piece��С
		uint64 num_blocks;		// piece������block��
		uint64 finished_blocks;	// ����block����
		ptime last_use;			// ���һ�ζ�/дʱ��
		bool verified;			// �Ƿ��Ѿ�����hashУ��

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

	// ������ͨ������Ӵ��̶�ȡ�����ݣ������ٶ����̴���
	CachedPieceSet read_cache_set_;
	boost::mutex read_cache_mutex_;

	// д����ͨ�������δ���д�������������д���̴���
	CachedPieceMap unfinished_write_cache_; // δ������ɵ�piece����
	CachedPieceList finished_write_cache_;  // ��������ɵ�piece����
	boost::mutex write_cache_mutex_;

	// �������ͳ��
	CacheStatus	cache_status_;

	// ��д�������
	uint64 read_cache_limit_;
	uint64 write_cache_limit_;
};

}

#endif