/*#############################################################################
 * �ļ���   : pps_torrent.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��25��
 * �ļ����� : PpsTorrent������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ������PpsЭ���FastResume��Ϣ
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
 *****************************************************************************/
struct PpsFastResume
{
	PpsFastResume() : total_size(0), piece_size(0) {}

	std::string desc;		// ������Ϣ
	std::string version;	// �汾��Ϣ
	InfoHashSP info_hash;	// ��ԴID
    std::string protocol;   // Э���ʶ
	uint64 total_size;		// ��Դ�ܴ�С
	uint64 piece_size;		// piece��С
	uint64 block_size;		// block��С
	uint64 metadata_size;   //metadata��С
	boost::dynamic_bitset<> piece_map;  //bitfield
    std::set<EndPoint> trackers;  //tracker-list
};

/******************************************************************************
 * ������PpsЭ���Metadata��Ϣ
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
 *****************************************************************************/
struct PpsMetadata
{
	PpsMetadata() : total_size(0), piece_size(0){}

	InfoHashSP info_hash;		// ������Դ��info-hash���ɼ������
	uint64 total_size;			// ������Դ�Ĵ�С
	uint64 piece_size;			// piece��Ƭ��С
};

/******************************************************************************
 * ����: PPSЭ��Torrent
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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
	// fastresume�������
	void SetFastResumeToBencode(BencodeEntry& entry);
	void GetFastResumeFromBencode(BencodeEntry& entry);
	void OnFastResumeRead(bool ok, const DiskIoJobSP& job);
	void OnFastResumeWritten(bool ok, const DiskIoJobSP& job);
	void ReadFastResume();
	bool WriteFastResume();
	bool CheckFastResume();

	// metadata�������
	void WriteMetadata(const std::string& metadata);
    void OnMetadataRead(bool ok, const DiskIoJobSP& job);
	void OnMetadataWritten(bool ok, const DiskIoJobSP& job);
	void TryRetriveMetadata();
	bool CheckMetadata(const std::string& metadata);
	void ReadMetadata();

	// baseinfo �������
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
	std::string metadata_origin_; // �������ȡ��metadataԭʼ����
	PpsMetadata pps_metadata_;
    MsgDispatcher msg_dispatcher_;

	bool support_proxy_;  //֧�ִ���

	bool baseinfo_ready_;  //��ȡbaseinfo���

};

}

#endif

