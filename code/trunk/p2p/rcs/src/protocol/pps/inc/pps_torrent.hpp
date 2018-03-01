/*#############################################################################
 * 文件名   : pps_torrent.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月25日
 * 文件描述 : PpsTorrent类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef PPS_TORRENT_HEAD
#define PPS_TORRENT_HEAD

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
class PpsMetadataRetriver;
class BtReplyProxyMsg;
class PpsReplyTrackerMsg;
struct PpsBaseInfoMsg;

/******************************************************************************
 * 描述：Pps协议的FastResume信息
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
struct PpsFastResume
{
	PpsFastResume() : total_size(0), piece_size(0) {}

	std::string desc;		// 描述信息
	std::string version;	// 版本信息
	InfoHashSP info_hash;	// 资源ID
    std::string protocol;   // 协议标识
	uint64 total_size;		// 资源总大小
	uint64 piece_size;		// piece大小
	uint64 block_size;		// block大小
	uint64 metadata_size;   //metadata大小
	boost::dynamic_bitset<> piece_map;  //bitfield
    std::set<EndPoint> trackers;  //tracker-list
};

/******************************************************************************
 * 描述：Pps协议的Metadata信息
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
struct PpsMetadata
{
	PpsMetadata() : total_size(0), piece_size(0){}

	InfoHashSP info_hash;		// 整个资源的info-hash，由计算得来
	uint64 total_size;			// 整个资源的大小
	uint64 piece_size;			// piece分片大小
};

/******************************************************************************
 * 描述: PPS协议Torrent
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsTorrent : public Torrent
{
public:
	PpsTorrent(Session& session, const InfoHashSP& hash, const fs::path& save_path);
	~PpsTorrent();
	
	virtual bool VerifyPiece(uint64 piece, const char* data, uint32 length) override;
	virtual void RetrivePeers() override;
	virtual void RetriveCachedRCSes() override;
	virtual void RetriveInnerProxies() override;
	virtual void RetriveOuterProxies() override;

	void RetrivedMetadata(const std::string& metadata);

	PpsMetadataRetriver* metadata_retriver() { return metadata_retriver_.get(); }

	std::string GetMetadata() const { return metadata_origin_; }

	void RetriveTrackerList();

	void AddBaseInfo(PpsBaseInfoMsg& msg);

	bool metadata_ready() { return metadata_ready_; }

private:
	virtual void Initialize() override;
	virtual void TickProc() override;
	virtual void HavePieceProc(uint64 piece) override;
    
private:
	// fastresume处理相关
	void SetFastResumeToBencode(BencodeEntry& entry);
	void GetFastResumeFromBencode(BencodeEntry& entry);
	void OnFastResumeRead(bool ok, const DiskIoJobSP& job);
	void OnFastResumeWritten(bool ok, const DiskIoJobSP& job);
	void ReadFastResume();
	bool WriteFastResume();
	bool CheckFastResume();

	// metadata处理相关
	void WriteMetadata(const std::string& metadata);
    void OnMetadataRead(bool ok, const DiskIoJobSP& job);
	void OnMetadataWritten(bool ok, const DiskIoJobSP& job);
	void TryRetriveMetadata();
	bool CheckMetadata(const std::string& metadata);
	void ReadMetadata();

	// baseinfo 处理相关
	void TryRetriveBaseInfo();
	
	void VerifyResource();
	void UpdateTorrentInfo(RetriveType retrive_type);
	void RetrievePeersFromTrackers();
	void RetrieveBaseInfoFromTrackers();
	void RefreshFastResumeFromBaseInfo(PpsBaseInfoMsg& base_info);

	void OnMsg(const PbMessage& msg);
    void OnReplyTrackerMsg(const PpsReplyTrackerMsg& msg);

	void set_retrieved_baseInfo(bool value) { baseinfo_ready_ = value; }

private:
    PpsFastResume fast_resume_;
	bool fastresume_updated_;
	
	boost::shared_ptr<PpsBaseinfoRetriver> baseinfo_retriver_;
	bool retrieving_baseinfo_;
	
	boost::scoped_ptr<PpsMetadataRetriver> metadata_retriver_;
    bool retrieving_peer_;

	bool metadata_ready_;
	std::string metadata_origin_; // 从网络获取的metadata原始数据
	PpsMetadata pps_metadata_;
    MsgDispatcher msg_dispatcher_;

	bool support_proxy_;  //支持代理

	bool baseinfo_ready_;  //获取baseinfo与否

};

}

#endif

