/*#############################################################################
 * �ļ���   : bt_torrent.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��13��
 * �ļ����� : BtTorrent��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bt_torrent.hpp"

#include "bc_util.hpp"
#include "bc_assert.hpp"
#include "rcs_config.hpp"
#include "bt_metadata_retriver.hpp"
#include "bt_tracker_request.hpp"
#include "communicator.hpp"
#include "info_hash.hpp"
#include "io_oper_manager.hpp"
#include "message.pb.h"
#include "session.hpp"
#include "tracker_request_manager.hpp"
#include "bt_pub.hpp"
#include "bt_udp_tracker_request.hpp"
#include "policy.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

//FR��fast resume����д
//fast-resume��bencode�����һ��mapʱ��ÿһ���������Ӧһ���ַ�����ʾ��key
static const char* const kFrDescription = "description";
static const char* const kFrVersion		= "version";
static const char* const kFrProtocol	= "protocol";
static const char* const kFrName		= "name";
static const char* const kFrInfoHash	= "info-hash";
static const char* const kFrTotalSize	= "total-size";
static const char* const kFrPieceSize	= "piece-size";
static const char* const kFrBlockSize	= "block-size";
static const char* const kFrPieceMap	= "piece-map";
static const char* const kFrHttpTrackerList = "http-tracker-list";
static const char* const kFrUdpTrackerList = "udp-tracker-list";

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��11��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtTorrent::BtTorrent(Session& session, const InfoHashSP& hash, const fs::path& save_path)
	: Torrent(session, hash, save_path)
	, fastresume_updated_(false)
	, retrieving_peer_(false)
{
	fast_resume_.desc = BT_FAST_RESUME_DESC;
	fast_resume_.version = BT_FAST_RESUME_VERSION;
	fast_resume_.protocol = BT_FAST_RESUME_PROTOCOL;

    msg_dispatcher_.RegisterCallback<BtReplyTrackerMsg>(
        boost::bind(&BtTorrent::OnReplyTrackerMsg, this, _1));
    msg_dispatcher_.RegisterCallback<BtReplyProxyMsg>(
        boost::bind(&BtTorrent::OnReplyRcsMsg, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��11��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
BtTorrent::~BtTorrent()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::Initialize()
{
	metadata_retriver_.reset(new BtMetadataRetriver(shared_from_this()));

	// �����ȡmetadata��Ŀ����ϣ����ȡmetadata�е�piece hash����ǰ�����Ϣ
	// ֻ������metadata�У���Ȼ���ܴ�ʱ��û��metadata������û��ϵ������
	// ��ȡʧ�ܣ������������ȡmetadata��Ҳ�ܽ�metadata���ص��ڴ档
	ReadMetadata();

	// �����fast-resume���ȡ�����򣬻ᴥ���������ȡmetadata
	ReadFastResume();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��11��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::TickProc()
{
	if (fastresume_updated_)
	{
		WriteFastResume();
		fastresume_updated_ = false;
	}

	//���metadata��û��ȡ�����͵���metadata�Ķ�ʱ��
	if (is_ready() == false)
	{
		metadata_retriver_->TickProc(); 
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������piece����
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::HavePieceProc(uint64 piece)
{
	BC_ASSERT(piece < fast_resume_.piece_map.size());

	fast_resume_.piece_map[piece] = true;

	fastresume_updated_ = true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�����peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::RetrivePeers()
{
    if (retrieving_peer_)  // ���ڻ�ȡpeer-list
        return ;

    retrieving_peer_ = true;  // �������ڻ�ȡpeer-list
	if (!fast_resume_.http_trackers.empty())  // fast-resume�ļ��б�����tracker-list
    { 
    	RetrievePeersFromTrackers();
    }
    else  // ��UGS��ȡtracker-list
    {
    	LOG(INFO)<<"Request tracker list...";	
        // ��������tracker��Ϣ
        BtRequestTrackerMsg msg;
        msg.set_info_hash(info_hash()->raw_data());

        // ��������http tracker��Ϣ
		msg.set_tracker_type(HTTP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&BtTorrent::OnMsg, this, _1));

        // ��������udp tracker��Ϣ
		msg.set_tracker_type(UDP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&BtTorrent::OnMsg, this, _1));
    }
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�Ѿ�������Դ��RCS
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::RetriveCachedRCSes()
{
    BtRequestProxyMsg msg;
    msg.set_rcs_type(CACHED_DATA_PROXY);
    msg.set_info_hash(info_hash()->raw_data());
    msg.set_num_want(1024);

    Communicator::GetInstance().SendMsg(msg, boost::bind(&BtTorrent::OnMsg, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ���ػ������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::RetriveInnerProxies()
{
    BtRequestProxyMsg msg;
    msg.set_rcs_type(INNER_PROXY);
    msg.set_info_hash(info_hash()->raw_data());
    msg.set_num_want(1024);

    Communicator::GetInstance().SendMsg(msg, boost::bind(&BtTorrent::OnMsg, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�ⲿ�������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::RetriveOuterProxies()
{
    BtRequestProxyMsg msg;
    msg.set_rcs_type(OUTER_PROXY);
    msg.set_info_hash(info_hash()->raw_data());
    msg.set_num_want(1024);

    Communicator::GetInstance().SendMsg(msg, boost::bind(&BtTorrent::OnMsg, this, _1));
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�����UGS����������Ϣ
 * ��  ��: [in] msg UGS����������Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.11
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtTorrent::OnMsg(const PbMessage& msg)
{
    msg_dispatcher_.Dispatch(msg);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����ugs�ظ���tracker��Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::OnReplyTrackerMsg(const BtReplyTrackerMsg& msg)
{
	if (msg.tracker_size() == 0) 
	{
		LOG(WARNING) << "Get no tracker from ugs";
	}
    LOG(INFO) << "tracker-list : ";
    std::vector<EndPoint> trackers;
    for(int i=0; i<msg.tracker_size(); ++i)
    {
        const EndPointStruct& ep = msg.tracker(i);
        EndPoint endpoint(ep.ip(), (unsigned short)(ep.port()));

        LOG(INFO) << endpoint; 
        if (msg.tracker_type() == HTTP_TRACKER)
			fast_resume_.http_trackers.insert(endpoint);
		else if (msg.tracker_type() == UDP_TRACKER)
			fast_resume_.udp_trackers.insert(endpoint);
    }

    RetrievePeersFromTrackers();
}

/*------------------------------------------------------------------------------
 * ��  ��: ����ugs�ظ���rcs��Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::OnReplyRcsMsg(const BtReplyProxyMsg& msg)
{
    RcsType rcs_type = msg.rcs_type();
    if (rcs_type == INNER_PROXY)
    {
        for (int i = 0; i < msg.proxy_size(); ++i)
        {
            const EndPointStruct& ep = msg.proxy(i);
            policy()->AddPeer(EndPoint(ep.ip(), static_cast<uint16>(ep.port())),
                    PEER_INNER_PROXY, PEER_RETRIVE_UGS);
        }
    }
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡMetadata��Ϣ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��26��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::TryRetriveMetadata()
{
	metadata_retriver_->Start();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��fast-resume��Ϣд��bencode������
 * ��  ��:
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.10.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::SetFastResumeToBencode(BencodeEntry& entry)
{
	entry[kFrDescription] = BencodeEntry(fast_resume_.desc);
	entry[kFrVersion]     = BencodeEntry(fast_resume_.version);
	entry[kFrProtocol]    = BencodeEntry(fast_resume_.protocol);
	entry[kFrName]        = BencodeEntry(fast_resume_.name);
	entry[kFrInfoHash]    = BencodeEntry(fast_resume_.info_hash->to_string());
	entry[kFrTotalSize]   = BencodeEntry(fast_resume_.total_size);
	entry[kFrPieceSize]   = BencodeEntry(fast_resume_.piece_size);
	entry[kFrBlockSize]   = BencodeEntry(fast_resume_.block_size);
	entry[kFrPieceMap]    = BencodeEntry(ToString(GetReversedBitset(fast_resume_.piece_map)));  // ��piece_mapת�����ַ���

    auto& http_trackers = entry[kFrHttpTrackerList].List();
    for (const EndPoint& tracker : fast_resume_.http_trackers)
    {
        http_trackers.push_back(BencodeEntry(to_string(tracker)));
	}    
	
	auto& udp_trackers = entry[kFrUdpTrackerList].List();
	for (const EndPoint& tracker : fast_resume_.udp_trackers)
	{
		udp_trackers.push_back(BencodeEntry(to_string(tracker)));
	}
}

/*------------------------------------------------------------------------------
 * ��  ��: ��bencode�����ж�ȡfast-resume��Ϣ
 * ��  ��:
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.10.16
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::GetFastResumeFromBencode(BencodeEntry& entry)
{
	fast_resume_.desc       = entry[kFrDescription].String();
	fast_resume_.version    = entry[kFrVersion].String();
	fast_resume_.protocol   = entry[kFrProtocol].String();
	fast_resume_.name       = entry[kFrName].String();
	fast_resume_.info_hash  = InfoHashSP(new BtInfoHash(FromHex(entry[kFrInfoHash].String())));
	fast_resume_.total_size = entry[kFrTotalSize].Integer();
	fast_resume_.piece_size = static_cast<uint64>(entry[kFrPieceSize].Integer());
	fast_resume_.block_size = static_cast<uint64>(entry[kFrBlockSize].Integer());
	fast_resume_.piece_map  = GetReversedBitset(boost::dynamic_bitset<>(entry[kFrPieceMap].String())); 

	auto& http_trackers = entry[kFrHttpTrackerList].List();
	for (const BencodeEntry& entry : http_trackers)
	{
        EndPoint endpoint;
        if (ParseEndPoint(entry.String(), endpoint))
        {
            fast_resume_.http_trackers.insert(endpoint);
        }
	}

	auto& udp_trackers = entry[kFrUdpTrackerList].List();
	for (const BencodeEntry& entry : udp_trackers)
	{
		EndPoint endpoint;
		if (ParseEndPoint(entry.String(), endpoint))
		{
			fast_resume_.udp_trackers.insert(endpoint);
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����metadata
 * ��  ��: [in] metadata ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��: 
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 *   ʱ�� 2013��10��8��
 *   ���� vicent_pan
 *   ��?ʵ��
 ----------------------------------------------------------------------------*/
bool BtTorrent::ParseMetadata(const std::string& metadata)
{
	std::string file_path;	

	// ͨ��info_hash�ж�metadata_cache�Ƿ���ȷ
	if (info_hash()->raw_data() != GetSha1(metadata.c_str(), metadata.size()))
	{
		LOG(ERROR) << "Invalid metadata" ;
		return false;
	}
	else
	{
		bt_metadata_.info_hash = info_hash();
	}

	LazyEntry bde_metadata;      // ����һ���յ�lazyEntry��Ϊ�洢���������ݡ�
	const char * data = metadata.c_str(); 
	if (LazyBdecode(data, data + (metadata.size() - 1), bde_metadata)) //LazyBdecode���롣
	{
		LOG(ERROR)<< "Fail to decode metadata cache";
		return false;
	}	  

	uint64 file_single_size = 0;   // �����ļ��Ĵ�С
	uint64 file_total_size = 0;    // ��Դ�������ļ����ܴ�С

	if (const LazyEntry*file_bde = bde_metadata.DictFindList("files")) // ���ļ���bendcode
	{
		int i = 0;

		while (i < file_bde->ListSize())
		{	
			file_single_size = file_bde->ListAt(i)->DictFindIntValue("length");
			file_path = file_bde->ListAt(i)->DictFindIntValue("path");
			bt_metadata_.files.push_back(MetadataFileEntry(file_path, file_single_size));
			file_total_size += file_single_size;
			i++;
		}

		bt_metadata_.total_size = file_total_size;
	}
	else  // ���ļ���bendcode
	{	
		file_single_size = bde_metadata.DictFindIntValue("length");

		// ���ڵ��ļ���˵���ļ���������Դ��
		bt_metadata_.files.push_back(MetadataFileEntry(std::string(), file_single_size));
		file_total_size = file_single_size ;
		bt_metadata_.total_size = file_total_size;
	}

	bt_metadata_.torrent_name = bde_metadata.DictFindStringValue("name");
	bt_metadata_.piece_size = bde_metadata.DictFindIntValue("piece length");
	std::string info_hashs = bde_metadata.DictFindStringValue("pieces");

	uint32 piece_num = CALC_FULL_MULT(bt_metadata_.total_size, bt_metadata_.piece_size);
	if (info_hashs.size() != BT_INFO_HASH_LEN * piece_num)
	{
		LOG(ERROR) << "Invalid piece infohash";
		return false;
	}

	bt_metadata_.piece_hash.resize(piece_num);
	for (uint32 i = 0; i < piece_num; ++i)
	{
		bt_metadata_.piece_hash[i].reset(new BtInfoHash(
			info_hashs.substr(i * BT_INFO_HASH_LEN, BT_INFO_HASH_LEN)));        
	}

	//TODO: ����Ҫ֧��metadataЭ�飬������ʱ�ȱ��浽torrent
	// ������metadata�Ĺ��������һ������
	metadata_origin_ = metadata;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����torrent��Ϣ
 * ��  ��: 
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::UpdateTorrentInfo(RetriveType retrive_type)
{
	TorrentInfo torrent_info;
	torrent_info.info_hash	  = fast_resume_.info_hash;
	torrent_info.total_size   = fast_resume_.total_size;
	torrent_info.piece_size   = fast_resume_.piece_size;
	torrent_info.block_size   = fast_resume_.block_size;
	torrent_info.piece_map    = fast_resume_.piece_map;
	torrent_info.retrive_type = retrive_type;
	
	// ֪ͨTorrent
	SetTorrentInfo(torrent_info);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������ȡ��metadata��Ĵ���
 * ��  ��: [in] metadata ����
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��10��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::RetrivedMetadata(const std::string& metadata)
{
	if (!ParseMetadata(metadata))
	{
		LOG(ERROR) << "Fail to parse metadata";
		return;
	}

	RefreshFastResumeFromMetadata();

	UpdateTorrentInfo(FROM_METADATA);

	WriteMetadata(metadata);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡmetadata�����fast-resume
 * ��  ��: 
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��10��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BtTorrent::RefreshFastResumeFromMetadata()
{
	BC_ASSERT(!fastresume_updated_);

	BencodeEntry entry;

	// ����fast-resume
	fast_resume_.name       = bt_metadata_.torrent_name;
	fast_resume_.info_hash  = bt_metadata_.info_hash;
	fast_resume_.total_size = bt_metadata_.total_size;
	fast_resume_.piece_size = bt_metadata_.piece_size;
	fast_resume_.block_size = BT_BLOCK_SIZE; //TODO: ����block��С��ȷ����������Ҫ��������
	fast_resume_.piece_map.resize(CALC_FULL_MULT(bt_metadata_.total_size, bt_metadata_.piece_size));

	entry[kFrDescription] = BencodeEntry(BT_FAST_RESUME_DESC);
	entry[kFrVersion]     = BencodeEntry(BT_FAST_RESUME_VERSION);
	entry[kFrProtocol]    = BencodeEntry(BT_FAST_RESUME_PROTOCOL);
	entry[kFrName]        = BencodeEntry(fast_resume_.name);
	entry[kFrInfoHash]    = BencodeEntry(fast_resume_.info_hash->to_string());
	entry[kFrTotalSize]   = BencodeEntry(fast_resume_.total_size);
	entry[kFrPieceSize]   = BencodeEntry(fast_resume_.piece_size);
	entry[kFrBlockSize]   = BencodeEntry(fast_resume_.block_size);
	entry[kFrPieceMap]    = BencodeEntry(ToString(fast_resume_.piece_map));  // ��piece_mapת�����ַ���
	entry[kFrHttpTrackerList];   // ����һ���յ�BencodeEntry
	entry[kFrUdpTrackerList];   // ����һ���յ�BencodeEntry

	fastresume_updated_ = true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��fast-resume��Ӧ��bencode��������д���ļ���
 * ��  ��: 
 * ����ֵ: д���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtTorrent::WriteFastResume()
{
	BencodeEntry entry;
    SetFastResumeToBencode(entry); //����fast-resume��Ӧ��bencode��������

	std::string entry_str = Bencode(entry);
     
    io_oper_manager()->AsyncWriteFastResume(
		boost::bind(&BtTorrent::OnFastResumeWritten, this, _1, _2), entry_str);

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡmetadata��Ϣ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.06
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::ReadMetadata()
{
    io_oper_manager()->AsyncReadMetadata(
		boost::bind(&BtTorrent::OnMetadataRead, this, _1, _2));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡfast-resume����
 * ��  ��:
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtTorrent::ReadFastResume()
{
    io_oper_manager()->AsyncReadFastResume(
		boost::bind(&BtTorrent::OnFastResumeRead, this, _1, _2));
}

/*------------------------------------------------------------------------------
 * ��  ��: fast-resume�ļ�д��ɺ�Ļص�����
 * ��  ��: [in] ok fast-resume�ļ�д�Ƿ�ɹ�
 *         [in] DiskIoJob ����һ��DiskIoJob��ɴ˴�д�����ģ�δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtTorrent::OnFastResumeWritten(bool ok, const DiskIoJobSP& job)
{
	WriteResumeJobSP write_job = SP_CAST(WriteResumeJob, job);

    if (!ok) //дfast-resumeʧ��
    {
        LOG(ERROR) << "Fail to write fast resume.\n";
    }
}

/*------------------------------------------------------------------------------
 * ��  ��: metadata��ȡ��ɺ�Ļص�����
 * ��  ��: [in] ok metadata��ȡ�Ƿ�ɹ�
 *         [in] job ��ɶ�ȡmetadata����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.06
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::OnMetadataRead(bool ok, const DiskIoJobSP& job)
{
	if (!ok)
	{
		LOG(INFO) << "Fail to read metadata from file."; 
		return;
	}

    ReadMetadataJobSP read_job = SP_CAST(ReadMetadataJob, job);
    ParseMetadata(read_job->metadata);  
}

/*------------------------------------------------------------------------------
 * ��  ��: У��fast-resume�����Ƿ�Ϸ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtTorrent::CheckFastResume()
{
	if ((fast_resume_.protocol != BT_FAST_RESUME_PROTOCOL)
		|| (fast_resume_.total_size == 0)
		|| (fast_resume_.piece_size == 0)
		|| (fast_resume_.block_size == 0))
	{
		return false;
	}

	// У����Դ��С
	if (fast_resume_.total_size > fast_resume_.piece_size * fast_resume_.piece_map.size())
	{
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: У����Դpiece-map
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtTorrent::VerifyResource()
{
	uint64 piece_num = fast_resume_.piece_map.size();
	uint64 common_piece_size = fast_resume_.piece_size;

	// ����ǰ���Ѿ�У���FastResume������������ᷭת
	uint64 last_piece_size = fast_resume_.total_size 
		- ((piece_num - 1) * common_piece_size);

	BC_ASSERT(last_piece_size <= common_piece_size);

	uint64 piece_size = 0;
	std::vector<char> piece_buf(common_piece_size);

	for (uint64 i = 0; i < piece_num; i++)
	{
		if (!fast_resume_.piece_map[i]) continue;

		if (i < piece_num - 1)
		{
			piece_size = common_piece_size;
		}
		else
		{
			piece_size = last_piece_size;
		}

		int ret = io_oper_manager()->ReadData(piece_buf.data(), i, 0, piece_size);
		if (ret < static_cast<int>(piece_size))
		{
			LOG(ERROR) << "Read data error";
			fast_resume_.piece_map[i] = false;
			continue;
		}

		if (!VerifyPiece(i, piece_buf.data(), piece_size))
		{
			LOG(ERROR) << "Fail to verify piece | " << i;
			fast_resume_.piece_map[i] = false;
		}
	}

	fastresume_updated_ = true;
}

/*------------------------------------------------------------------------------
 * ��  ��: fast-resume�ļ���ȡ��ɺ�Ļص�����
 * ��  ��: [in] ok fast-resume�ļ���ȡ�Ƿ�ɹ�
 *         [in] DiskIoJob ����һ��DiskIoJob��ɴ˴ζ�ȡ�����ģ�δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtTorrent::OnFastResumeRead(bool ok, const DiskIoJobSP& job)
{
	ReadResumeJobSP read_job = SP_CAST(ReadResumeJob, job);

	// �����ȡʧ�ܣ���������ȡmetadata
	if (!ok || read_job->fast_resume.size() == 0)
	{
		LOG(INFO) << "Fail to read fast-resume from file";
		TryRetriveMetadata();
		return;
	}
	else
	{
		LOG(INFO) << "Success to read fast-resume from file";
	}

	// ����fast-resume
	BencodeEntry entry;
	entry = Bdecode(read_job->fast_resume); 
	GetFastResumeFromBencode(entry);

	// У��fast-resume��Ϣ��������Ϸ���������ȡ	
	if (!CheckFastResume())
	{
		LOG(ERROR) << "Invalid fast-resume data";
		TryRetriveMetadata();
		return;
	}

	// ֪ͨtorrentһ��OK
	UpdateTorrentInfo(FROM_FASTRESUME);

	// fast-resume��ȡ�ɹ������ݺϷ�������ҪУ����Դ����
	VerifyResource();
}

/*------------------------------------------------------------------------------
 * ��  ��: metadataд���ļ�
 * ��  ��: [in] buf ����
 *         [in] len ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��19��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtTorrent::WriteMetadata(const std::string& metadata)
{
	io_oper_manager()->AsyncWriteMetadata(
		boost::bind(&BtTorrent::OnMetadataWritten, this, _1, _2), metadata);
}

/*------------------------------------------------------------------------------
 * ��  ��: metadata�ļ�д����ɺ�Ļص�����
 * ��  ��: [in] ok metadata�ļ�д���Ƿ�ɹ�
 *         [in] job ����һ��DiskIoJob��ɴ˴ζ�ȡ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��19��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void BtTorrent::OnMetadataWritten(bool ok, const DiskIoJobSP& job)
{
	if (ok)
	{
		LOG(INFO) << "Write metadata successfully";
	}
	else
	{
		LOG(ERROR) << "Fail to write metadata";
	}
}

/*------------------------------------------------------------------------------
 * ��  ��: У��piece��info-hash�Ƿ���ȷ
 * ��  ��: [in] piece ��У��piece������
 *         [in] data piece��Ӧ������ָ��
 *         [in] length piece��Ӧ�����ݳ���
 * ����ֵ: info-hash�Ƿ���ȷ
 * ��  ��:
 *   ʱ�� 2013.11.06
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool BtTorrent::VerifyPiece(uint64 piece, const char* data, uint32 length)
{
    return bt_metadata_.piece_hash[piece]->raw_data() == GetSha1(data, length);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��tracker��������ȡpeer-list 
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.13
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void BtTorrent::RetrievePeersFromTrackers()
{
    TrackerRequestManager& tr_manager = TrackerRequestManager::GetInstance();
    for (const EndPoint& tracker : fast_resume_.http_trackers)
    {	
	   	BtTrackerRequestSP request(new BtTrackerRequest(session(), 
            shared_from_this(), tracker));
       tr_manager.AddRequest(request);
    }
	
	for (const EndPoint& tracker : fast_resume_.udp_trackers)
    {
		BtUdpTrackerRequestSP udp_request(new BtUdpTrackerRequest(session(), 
		    shared_from_this(), tracker));
		tr_manager.AddRequest(udp_request);   // start udp tracker
    }

	retrieving_peer_ = false;
}

}
