/*#############################################################################
 * �ļ���   : bt_torrent.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��13��
 * �ļ����� : BtTorrent������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ������BTЭ���FastResume��Ϣ
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/09
 *****************************************************************************/
struct BtFastResume
{
	BtFastResume() : total_size(0), piece_size(0) {}

	std::string desc;		// ������Ϣ
	std::string version;	// �汾��Ϣ
	InfoHashSP info_hash;	// ��ԴID
	std::string name;		// ��Դ����
    std::string protocol;   // Э���ʶ
	uint64 total_size;		// ��Դ�ܴ�С
	uint64 piece_size;		// piece��С
	uint64 block_size;		// block��С
	boost::dynamic_bitset<> piece_map;  // bitfield
    std::set<EndPoint> http_trackers;  // http_tracker-list
    std::set<EndPoint> udp_trackers; //udp_tracker_list
};

/******************************************************************************
 * ������BTЭ��Metadata��Ϣ����Э��ģ������ڴ˻�������չ
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/09
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

	InfoHashSP info_hash;		// ������Դ��info-hash���ɼ������
	uint64 total_size;			// ������Դ�Ĵ�С
	uint64 piece_size;			// piece��Ƭ��С
	bool is_private;			// private��־���������׼Э��
	std::string torrent_name;	// ��Դ�ļ���
	std::vector<InfoHashSP> piece_hash;
	std::vector<MetadataFileEntry> files;
};

/******************************************************************************
 * ����: BTЭ��Torrent
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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

	// fast-resume�������
	void SetFastResumeToBencode(BencodeEntry& entry);
	void GetFastResumeFromBencode(BencodeEntry& entry);
	void OnFastResumeRead(bool ok, const DiskIoJobSP& job);
	void OnFastResumeWritten(bool ok, const DiskIoJobSP& job);
	bool WriteFastResume();
	void RefreshFastResumeFromMetadata();
	void ReadFastResume();
	bool CheckFastResume();

	// metadata�������
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

	std::string metadata_origin_; // �������ȡ��metadataԭʼ����
	BtMetadata bt_metadata_;
    MsgDispatcher msg_dispatcher_;
};

}
