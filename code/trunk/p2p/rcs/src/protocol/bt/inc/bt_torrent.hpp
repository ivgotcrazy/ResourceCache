/*#############################################################################
 * 文件名   : bt_torrent.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月13日
 * 文件描述 : BtTorrent类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <vector>
#include <string>
#include "torrent.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "msg_dispatcher.hpp"

namespace BroadCache
{

class Session;
class InfoHash;
class BtMetadataRetriver;
class BtReplyProxyMsg;
class BtReplyTrackerMsg;

/******************************************************************************
 * 描述：BT协议的FastResume信息
 * 作者：teck_zhou
 * 时间：2013/09/09
 *****************************************************************************/
struct BtFastResume
{
	BtFastResume() : total_size(0), piece_size(0) {}

	std::string desc;		// 描述信息
	std::string version;	// 版本信息
	InfoHashSP info_hash;	// 资源ID
	std::string name;		// 资源名称
    std::string protocol;   // 协议标识
	uint64 total_size;		// 资源总大小
	uint64 piece_size;		// piece大小
	uint64 block_size;		// block大小
	boost::dynamic_bitset<> piece_map;  // bitfield
    std::set<EndPoint> http_trackers;  // http_tracker-list
    std::set<EndPoint> udp_trackers; //udp_tracker_list
};

/******************************************************************************
 * 描述：BT协议Metadata信息，各协议模块可以在此基础上扩展
 * 作者：teck_zhou
 * 时间：2013/09/09
 *****************************************************************************/
struct MetadataFileEntry
{
	MetadataFileEntry(std::string path, uint64 length) 
		: file_path(path), file_length(length) {}

	std::string file_path;
	uint64 file_length;
	
	// MD5SUM
};

struct BtMetadata
{
	BtMetadata() : total_size(0), piece_size(0), is_private(false) {}

	InfoHashSP info_hash;		// 整个资源的info-hash，由计算得来
	uint64 total_size;			// 整个资源的大小
	uint64 piece_size;			// piece分片大小
	bool is_private;			// private标志，具体见标准协议
	std::string torrent_name;	// 资源文件名
	std::vector<InfoHashSP> piece_hash;
	std::vector<MetadataFileEntry> files;
};

/******************************************************************************
 * 描述: BT协议Torrent
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtTorrent : public Torrent
{
public:
	BtTorrent(Session& session, const InfoHashSP& hash, const fs::path& save_path);
	~BtTorrent();
	
	virtual bool VerifyPiece(uint64 piece, const char* data, uint32 length) override;
	virtual void RetrivePeers() override;
	virtual void RetriveCachedRCSes() override;
	virtual void RetriveInnerProxies() override;
	virtual void RetriveOuterProxies() override;

	void RetrivedMetadata(const std::string& metadata);

	BtMetadataRetriver* metadata_retriver() { return metadata_retriver_.get(); }

	std::string GetMetadata() const { return metadata_origin_; }

private:
	virtual void Initialize() override;
	virtual void TickProc() override;
	virtual void HavePieceProc(uint64 piece) override;
    
private:

	// fast-resume处理相关
	void SetFastResumeToBencode(BencodeEntry& entry);
	void GetFastResumeFromBencode(BencodeEntry& entry);
	void OnFastResumeRead(bool ok, const DiskIoJobSP& job);
	void OnFastResumeWritten(bool ok, const DiskIoJobSP& job);
	bool WriteFastResume();
	void RefreshFastResumeFromMetadata();
	void ReadFastResume();
	bool CheckFastResume();

	// metadata处理相关
	void WriteMetadata(const std::string& metadata);
    void OnMetadataRead(bool ok, const DiskIoJobSP& job);
	void OnMetadataWritten(bool ok, const DiskIoJobSP& job);
	void TryRetriveMetadata();
	bool ParseMetadata(const std::string& metadata);
	void ReadMetadata();
	
	void VerifyResource();
	void UpdateTorrentInfo(RetriveType retrive_type);
	void RetrievePeersFromTrackers();

    void OnMsg(const PbMessage& msg);
    void OnReplyTrackerMsg(const BtReplyTrackerMsg& msg);
    void OnReplyRcsMsg(const BtReplyProxyMsg& msg);

private:
    BtFastResume fast_resume_;
	bool fastresume_updated_;

	boost::scoped_ptr<BtMetadataRetriver> metadata_retriver_;
	
    bool retrieving_peer_;

	std::string metadata_origin_; // 从网络获取的metadata原始数据
	BtMetadata bt_metadata_;
    MsgDispatcher msg_dispatcher_;
};

}
