/*#############################################################################		
 * 文件名   : torrent.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年08月08日
 * 文件描述 : Torrent类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_TORRENT
#define HEADER_TORRENT

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "sha1.hpp"
#include "endpoint.hpp"
#include "peer.hpp"
#include "torrent_file.hpp"
#include "file.hpp"
#include "bencode.hpp"
#include "bc_util.hpp"
#include "lazy_entry.hpp"
#include "connection.hpp"


namespace BroadCache
{

class Session;
class PeerConnection;
class InfoHash;
class PiecePicker;
class IoOperManager;
class Policy;
class Peer;
class DiskIoJob;
class Timer;

/******************************************************************************
 * 描述：表示一个下载资源，管理与整个资源相关的一些信息
 * 作者：teck_zhou
 * 时间：2013/08/21
 *****************************************************************************/
class Torrent : public boost::noncopyable, 
	            public boost::enable_shared_from_this<Torrent>
{
public:	
	Torrent(Session& session, 
		    const InfoHashSP& hash, 
			const fs::path& save_path);

	virtual ~Torrent();

	bool Init();
	void HavePiece(uint64 piece);

	void SetSeed();
	void SetProxy();

	// 阻塞主动连接，用于资源监控控制 
	void StopLaunchNewConnection();
	void ResumeLaunchNewConnection();

	uint64 GetPieceSize(uint64 index) const;
	uint64 GetBlockSize(uint64 piece, uint64 block) const;
	uint64 GetCommonPieceSize() const;
	uint64 GetCommonBlockSize() const;
	uint64 GetLastPieceSize() const;
	uint64 GetLastBlockSize(uint64 piece) const;
	uint64 GetPieceNum() const;
	uint64 GetBlockNum(uint64 piece) const;	

	bool is_ready() const { return is_ready_; }
	bool is_seed() const { return is_seed_; }
	bool is_proxy() const { return is_proxy_; }
	bool IsInactive() const;

	Policy* policy() { return policy_.get(); }
	Session& session() { return session_; }
	IoOperManager* io_oper_manager() const { return io_oper_manager_.get(); }
	PiecePicker* piece_picker() const { return piece_picker_.get(); }
	InfoHashSP info_hash() { return torrent_info_.info_hash; }
	fs::path save_path() const { return save_path_; }
	uint32 alive_time() const { return alive_time_; }

	virtual bool VerifyPiece(uint64 piece, const char* data, uint32 length) = 0;
	virtual void RetrivePeers() = 0;
	virtual void RetriveCachedRCSes() = 0;
	virtual void RetriveInnerProxies() = 0;
	virtual void RetriveOuterProxies() = 0;

protected:

	enum RetriveType
	{
		FROM_FASTRESUME,
		FROM_METADATA,
		FROM_BASEINFO
	};

	struct TorrentInfo
	{
		InfoHashSP info_hash;
		uint64 total_size;
		uint64 piece_size;
		uint64 block_size;
		boost::dynamic_bitset<> piece_map;
		RetriveType retrive_type;
	};

	// 协议对象获取torrent元信息后调用此接口设置
	void SetTorrentInfo(const TorrentInfo& torrent_info);

private:
	virtual void Initialize() = 0;
	virtual void TickProc() = 0;	
	virtual void HavePieceProc(uint64 piece) = 0;

private:
	void OnTick();
	void UpdateAliveTimeWithoutConn();

private:
	Session& session_;
	boost::shared_ptr<Policy> policy_;		
	boost::scoped_ptr<IoOperManager> io_oper_manager_;	
	boost::scoped_ptr<PiecePicker> piece_picker_;
	
	fs::path save_path_; // torrent保存路径

	uint32 alive_time_; //记录存活时间

	TorrentInfo	torrent_info_; //torrent下的资源信息  

	bool is_ready_;	// 是否已经获取metadata
	bool is_seed_;	// 做种标记
	
	// 是否是缓存代理，如果是缓存代理，则当所有分布式下载连接
	// 关闭后，torrent所关联的所有文件都会被删除。
	bool is_proxy_;	

	// 已经有这么长时间没有连接了
	uint32 alive_time_without_conn_; 

	// 超过这么长时间没有连接，就要将torrent移除
	uint32 max_alive_time_without_conn_; 

	// 每个torrent使用独立的timer
	boost::scoped_ptr<Timer> timer_;
		
	friend class GetBcrmInfo;
};

}

#endif
