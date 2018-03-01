/*#############################################################################
 * �ļ���   : pps_torrent.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��30��
 * �ļ����� : PpsTorrent��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_torrent.hpp"

#include "bc_util.hpp"
#include "bc_assert.hpp"
#include "rcs_config.hpp"
#include "communicator.hpp"
#include "info_hash.hpp"
#include "io_oper_manager.hpp"
#include "message.pb.h"
#include "session.hpp"
#include "tracker_request_manager.hpp"
#include "pps_pub.hpp"
#include "policy.hpp"
#include "rcs_util.hpp"
#include "pps_metadata_retriver.hpp"
#include "pps_baseinfo_retriver.hpp"
#include "pps_tracker_request.hpp"
#include "pps_baseinfo_tracker_request.hpp"

namespace BroadCache
{

//FR��fast resume����д
//fast-resume��bencode�����һ��mapʱ��ÿһ���������Ӧһ���ַ�����ʾ��key
static const char* const kFrDescription 	= "description";
static const char* const kFrVersion			= "version";
static const char* const kFrProtocol		= "protocol";
static const char* const kFrName			= "name";
static const char* const kFrInfoHash		= "info-hash";
static const char* const kFrTotalSize		= "total-size";
static const char* const kFrPieceSize		= "piece-size";
static const char* const kFrBlockSize		= "block-size";
static const char* const kFrMetadataSize	= "metadata-size";
static const char* const kFrPieceMap		= "piece-map";
static const char* const kFrTrackerList 	= "tracker-list";

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��11��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsTorrent::PpsTorrent(Session& session, const InfoHashSP& hash, const fs::path& save_path)
	: Torrent(session, hash, save_path)
	, fastresume_updated_(false)
	, retrieving_peer_(false)
	, metadata_ready_(false)
	, baseinfo_ready_(true)
{
	fast_resume_.desc = PPS_FAST_RESUME_DESC;
	fast_resume_.version = PPS_FAST_RESUME_VERSION;
	fast_resume_.protocol = PPS_FAST_RESUME_PROTOCOL;

    msg_dispatcher_.RegisterCallback<PpsReplyTrackerMsg>(
        boost::bind(&PpsTorrent::OnReplyTrackerMsg, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��11��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
PpsTorrent::~PpsTorrent()
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::Initialize()
{
	baseinfo_retriver_.reset(new PpsBaseinfoRetriver(shared_from_this()));

	metadata_retriver_.reset(new PpsMetadataRetriver(shared_from_this()));

	// �����ȡmetadata��Ŀ����ϣ����ȡmetadata�е�piece hash����ǰ�����Ϣ
	// ֻ������metadata�У���Ȼ���ܴ�ʱ��û��metadata������û��ϵ������
	// ��ȡʧ�ܣ������������ȡmetadata��Ҳ�ܽ�metadata���ص��ڴ档
	//ReadMetadata();  

	// �����fast-resume���ȡ�����򣬻ᴥ���������ȡBaseInfo
	ReadFastResume();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��11��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::TickProc()
{
	if (fastresume_updated_)
	{
		WriteFastResume();
		fastresume_updated_ = false;
	}

	//���baseinfo��û��ȡ���͵���RetriveBaseInfo
	if (!baseinfo_ready_)
	{	
		baseinfo_retriver_->TickProc();
	}
	
	//���metadata��û��ȡ�����͵���metadata�Ķ�ʱ��
	if (baseinfo_ready_ && metadata_ready_ == false)
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
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::HavePieceProc(uint64 piece)
{
	BC_ASSERT(piece < fast_resume_.piece_map.size());

	fast_resume_.piece_map[piece] = true;

	fastresume_updated_ = true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��BaseinfoRetriver��ȡTracker��ַ�ӿ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��23��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::RetriveTrackerList()
{
	LOG(INFO) << "Retrive Tracker List";

	if (!fast_resume_.trackers.empty())  // fast-resume�ļ��б�����tracker-list
    { 
    	baseinfo_retriver_->AddTrackerList(fast_resume_.trackers);
    }
    else  // ��UGS��ȡtracker-list
    {
    	LOG(INFO)<<"Request tracker list for retrive base info ...";	
		
        // ��������tracker��Ϣ
        PpsRequestTrackerMsg msg;
        msg.set_info_hash(info_hash()->raw_data());

        // ��������udp tracker��Ϣ
		msg.set_tracker_type(UDP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&PpsTorrent::OnMsg, this, _1));
    }
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ�����peer
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014.1.3
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::RetrivePeers()
{
	if (!baseinfo_ready_) // baseinfoû�л�ȡ��ֱ���˳�
		return; 
	
    if (retrieving_peer_)  // ���ڻ�ȡpeer-list
        return ;

	LOG(INFO) << "Policy call retrive peer";

    retrieving_peer_ = true;  // �������ڻ�ȡpeer-list
	if (!fast_resume_.trackers.empty())  // fast-resume�ļ��б�����tracker-list
    { 
    	RetrievePeersFromTrackers();
    }
    else  // ��UGS��ȡtracker-list
    {
    	LOG(INFO)<<"Request tracker list";	
        // ��������tracker��Ϣ
        PpsRequestTrackerMsg msg;
        msg.set_info_hash(info_hash()->raw_data());

        // ��������udp tracker��Ϣ
		msg.set_tracker_type(UDP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&PpsTorrent::OnMsg, this, _1));
    }
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�Ѿ�������Դ��RCS
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::RetriveCachedRCSes()
{
    BtRequestProxyMsg msg;
    msg.set_rcs_type(CACHED_DATA_PROXY);
    msg.set_info_hash(info_hash()->raw_data());
    msg.set_num_want(1024);

    Communicator::GetInstance().SendMsg(msg, boost::bind(&PpsTorrent::OnMsg, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ���ػ������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::RetriveInnerProxies()
{
    BtRequestProxyMsg msg;
    msg.set_rcs_type(INNER_PROXY);
    msg.set_info_hash(info_hash()->raw_data());
    msg.set_num_want(1024);

    Communicator::GetInstance().SendMsg(msg, boost::bind(&PpsTorrent::OnMsg, this, _1));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�ⲿ�������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::RetriveOuterProxies()
{
    BtRequestProxyMsg msg;
    msg.set_rcs_type(OUTER_PROXY);
    msg.set_info_hash(info_hash()->raw_data());
    msg.set_num_want(1024);

    Communicator::GetInstance().SendMsg(msg, boost::bind(&PpsTorrent::OnMsg, this, _1));
}

/*------------------------------------------------------------------------------
 * ��  ��: �յ�����UGS����������Ϣ
 * ��  ��: [in] msg UGS����������Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.11
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsTorrent::OnMsg(const PbMessage& msg)
{
    msg_dispatcher_.Dispatch(msg);
}

/*------------------------------------------------------------------------------
 * ��  ��: ����ugs�ظ���tracker��Ϣ
 * ��  ��: [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.23
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::OnReplyTrackerMsg(const PpsReplyTrackerMsg& msg)
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
		
		if (msg.tracker_type() == UDP_TRACKER)
			fast_resume_.trackers.insert(endpoint);
    }
	
	if (!baseinfo_ready_) //���û�л�ȡbaseinfo �Ȼ�ȡbaseinfo
		baseinfo_retriver_->AddTrackerList(fast_resume_.trackers);
	else
    	RetrievePeersFromTrackers();
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
void PpsTorrent::TryRetriveMetadata()
{
	LOG(INFO) << "Try Retrive Metadata";

	LOG(INFO) << "Metadata size : " << fast_resume_.metadata_size;

	//Todo �� fastresume��ȡ�����ж�
	metadata_retriver_->set_metadata_size(fast_resume_.metadata_size);
		
	metadata_retriver_->Start();
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
void PpsTorrent::TryRetriveBaseInfo()
{
	set_retrieved_baseInfo(false);
	baseinfo_retriver_->Start();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��fast-resume��Ϣд��bencode������
 * ��  ��:
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.12.30
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::SetFastResumeToBencode(BencodeEntry& entry)
{
	entry[kFrDescription]  = BencodeEntry(fast_resume_.desc);
	entry[kFrVersion]      = BencodeEntry(fast_resume_.version);
	entry[kFrProtocol]     = BencodeEntry(fast_resume_.protocol);
	entry[kFrInfoHash]     = BencodeEntry(fast_resume_.info_hash->to_string());
	entry[kFrTotalSize]    = BencodeEntry(fast_resume_.total_size);
	entry[kFrPieceSize]    = BencodeEntry(fast_resume_.piece_size);
	entry[kFrBlockSize]    = BencodeEntry(fast_resume_.block_size);
	entry[kFrMetadataSize] = BencodeEntry(fast_resume_.metadata_size);
	entry[kFrPieceMap]     = BencodeEntry(ToString(GetReversedBitset(fast_resume_.piece_map)));  // ��piece_mapת�����ַ���
	
	auto& udp_trackers = entry[kFrTrackerList].List();
	for (const EndPoint& tracker : fast_resume_.trackers)
	{
		udp_trackers.push_back(BencodeEntry(to_string(tracker)));
	}
}

/*------------------------------------------------------------------------------
 * ��  ��: ��bencode�����ж�ȡfast-resume��Ϣ
 * ��  ��:
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.12.30
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::GetFastResumeFromBencode(BencodeEntry& entry)
{
	fast_resume_.desc       = entry[kFrDescription].String();
	fast_resume_.version    = entry[kFrVersion].String();
	fast_resume_.protocol   = entry[kFrProtocol].String();
	fast_resume_.info_hash  = InfoHashSP(new BtInfoHash(FromHex(entry[kFrInfoHash].String())));
	fast_resume_.total_size = entry[kFrTotalSize].Integer();
	fast_resume_.piece_size = static_cast<uint64>(entry[kFrPieceSize].Integer());
	fast_resume_.block_size = static_cast<uint64>(entry[kFrBlockSize].Integer());
	fast_resume_.metadata_size = static_cast<uint64>(entry[kFrMetadataSize].Integer());
	fast_resume_.piece_map  = GetReversedBitset(boost::dynamic_bitset<>(entry[kFrPieceMap].String())); 

	auto& udp_trackers = entry[kFrTrackerList].List();
	for (const BencodeEntry& entry : udp_trackers)
	{
		EndPoint endpoint;
		if (ParseEndPoint(entry.String(), endpoint))
		{
			fast_resume_.trackers.insert(endpoint);
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����torrent��Ϣ
 * ��  ��: 
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��11��07��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::UpdateTorrentInfo(RetriveType retrive_type)
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
 * ��  ��: ���Baseinfo�Ľӿ�
 * ��  ��: 
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��12��30��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::AddBaseInfo(PpsBaseInfoMsg& msg)
{
	if (fastresume_updated_) return; 
	
	LOG(INFO) << " Add base info ";

	set_retrieved_baseInfo(true);

	//����fastresume�е���Ϣ
	RefreshFastResumeFromBaseInfo(msg);

	//����torrentinfo�е���Ϣ
	UpdateTorrentInfo(FROM_BASEINFO);	//ugs proto�ļ���Ҫ������

	//��ȡpeer-list
	RetrievePeersFromTrackers();

	//metadata_retriver�� torrentinfo���º�����
	TryRetriveMetadata();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡbaseinfo�����fast-resume
 * ��  ��: 
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��10��19��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::RefreshFastResumeFromBaseInfo(PpsBaseInfoMsg& base_info)
{
	BC_ASSERT(!fastresume_updated_);

	BencodeEntry entry;

	// ����fast-resume
	//fast_resume_.name       = base_info.torrent_name;
	fast_resume_.info_hash  = info_hash();
	fast_resume_.total_size = base_info.file_size;
	fast_resume_.piece_size = CalcPpsPieceSize(base_info.file_size, base_info.piece_count);
	fast_resume_.block_size = PPS_BLOCK_SIZE;  //pps��block��СΪ1024
	fast_resume_.metadata_size = base_info.metadata_size;
	fast_resume_.piece_map.resize(base_info.piece_count);
	

	LOG(INFO) << "file_size: " << base_info.file_size;
	LOG(INFO) << "piece_count: " << base_info.piece_count;
	LOG(INFO) << "bitfiled_size: " << base_info.bitfield_size;
	LOG(INFO) << "metadata_size: " << base_info.metadata_size;
	LOG(INFO) << "piece_size: " << fast_resume_.piece_size;

	entry[kFrDescription] = BencodeEntry(PPS_FAST_RESUME_DESC);
	entry[kFrVersion]     = BencodeEntry(PPS_FAST_RESUME_VERSION);
	entry[kFrProtocol]    = BencodeEntry(PPS_FAST_RESUME_PROTOCOL);
	//entry[kFrName]        = BencodeEntry(fast_resume_.name);
	entry[kFrInfoHash]    = BencodeEntry(fast_resume_.info_hash->to_string());
	entry[kFrTotalSize]   = BencodeEntry(fast_resume_.total_size);
	entry[kFrPieceSize]   = BencodeEntry(fast_resume_.piece_size);
	entry[kFrBlockSize]   = BencodeEntry(fast_resume_.block_size);
	entry[kFrPieceMap]    = BencodeEntry(ToString(fast_resume_.piece_map));  // ��piece_mapת�����ַ���
	entry[kFrTrackerList];   // ����һ���յ�BencodeEntry

	fastresume_updated_ = true;
}



/*-----------------------------------------------------------------------------
 * ��  ��: �������ȡ��metadata��Ĵ���
 * ��  ��: [in] metadata ����
 * ����ֵ: 
 * ��  ��: 
 *   ʱ�� 2013��10��19��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PpsTorrent::RetrivedMetadata(const std::string& metadata)
{
	//ֻ����metadata���ȵ��ж�
	if (!CheckMetadata(metadata)) return;
	
	WriteMetadata(metadata);

	metadata_ready_ = true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��fast-resume��Ӧ��bencode��������д���ļ���
 * ��  ��: 
 * ����ֵ: д���Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsTorrent::WriteFastResume()
{
	BencodeEntry entry;
    SetFastResumeToBencode(entry); //����fast-resume��Ӧ��bencode��������

	std::string entry_str = Bencode(entry);
     
    io_oper_manager()->AsyncWriteFastResume(
		boost::bind(&PpsTorrent::OnFastResumeWritten, this, _1, _2), entry_str);

    return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡfast-resume����
 * ��  ��:
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsTorrent::ReadFastResume()
{
    io_oper_manager()->AsyncReadFastResume(
		boost::bind(&PpsTorrent::OnFastResumeRead, this, _1, _2));
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
void PpsTorrent::ReadMetadata()
{
    io_oper_manager()->AsyncReadMetadata(
		boost::bind(&PpsTorrent::OnMetadataRead, this, _1, _2));
}


/*------------------------------------------------------------------------------
 * ��  ��: fast-resume�ļ�д��ɺ�Ļص�����
 * ��  ��: [in] ok fast-resume�ļ�д�Ƿ�ɹ�
 *         [in] DiskIoJob ����һ��DiskIoJob��ɴ˴�д�����ģ�δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsTorrent::OnFastResumeWritten(bool ok, const DiskIoJobSP& job)
{
	WriteResumeJobSP write_job = SP_CAST(WriteResumeJob, job);

    if (!ok) //дfast-resumeʧ��
    {
        LOG(ERROR) << "Fail to write fast resume.\n";
    }
}

bool PpsTorrent::CheckMetadata(const std::string& metadata)
{
	LOG(INFO) << "metadata size: " << metadata.size() << "fast resume size: " 
			<< fast_resume_.metadata_size;
		
	if (metadata.size() != fast_resume_.metadata_size) return false;

	//����metadta����
	metadata_ready_ = true;

	return true;
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
void PpsTorrent::OnMetadataRead(bool ok, const DiskIoJobSP& job)
{
	ReadMetadataJobSP read_job = SP_CAST(ReadMetadataJob, job);

	LOG(INFO) << "On metadata read";
	
	// �����ȡʧ�ܣ���������ȡmetadata
	if (!ok || read_job->metadata.size() == 0)
	{
		LOG(INFO) << "Fail to read metadata from file";
		TryRetriveMetadata();
		return;
	}

	if (!CheckMetadata(read_job->metadata))
	{
		TryRetriveMetadata();
	}
}

/*------------------------------------------------------------------------------
 * ��  ��: У��fast-resume�����Ƿ�Ϸ�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsTorrent::CheckFastResume()
{
	if ((fast_resume_.protocol != PPS_FAST_RESUME_PROTOCOL)
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
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsTorrent::VerifyResource()
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
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsTorrent::OnFastResumeRead(bool ok, const DiskIoJobSP& job)
{
	ReadResumeJobSP read_job = SP_CAST(ReadResumeJob, job);

	// �����ȡʧ�ܣ���������ȡmetadata
	if (!ok || read_job->fast_resume.size() == 0)
	{
		LOG(INFO) << "Fail to read fast-resume from file";
		TryRetriveBaseInfo();
		return;
	}
	else
	{
		LOG(INFO) << "Success to read fast-resume from file";
	}

	//����metadata�Ƿ��л�ȡ
	ReadMetadata();  	

	// ����fast-resume
	BencodeEntry entry;
	entry = Bdecode(read_job->fast_resume); 
	GetFastResumeFromBencode(entry);

	// У��fast-resume��Ϣ��������Ϸ���������ȡ	
	if (!CheckFastResume())
	{
		LOG(ERROR) << "Invalid fast-resume data";
		TryRetriveBaseInfo();  //�˴���Pps�ǻ�ȡbaseinfo
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
 *   ʱ�� 2014��1��3��
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsTorrent::WriteMetadata(const std::string& metadata)
{
	io_oper_manager()->AsyncWriteMetadata(
		boost::bind(&PpsTorrent::OnMetadataWritten, this, _1, _2), metadata);
}

/*------------------------------------------------------------------------------
 * ��  ��: metadata�ļ�д����ɺ�Ļص�����
 * ��  ��: [in] ok metadata�ļ�д���Ƿ�ɹ�
 *         [in] job ����һ��DiskIoJob��ɴ˴ζ�ȡ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��19��
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PpsTorrent::OnMetadataWritten(bool ok, const DiskIoJobSP& job)
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
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PpsTorrent::VerifyPiece(uint64 piece, const char* data, uint32 length)
{
    return true;  //pps����piece��У�飬������metadata
}

/*------------------------------------------------------------------------------
 * ��  ��: ��tracker��������ȡpeer-list 
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014.1.3
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::RetrievePeersFromTrackers()
{
	LOG(INFO) << "Retrive Peers Tracker";
    TrackerRequestManager& tr_manager = TrackerRequestManager::GetInstance();
	
	for (const EndPoint& tracker : fast_resume_.trackers)
    {
		PpsTrackerRequestSP request(new PpsTrackerRequest(session(), 
		    shared_from_this(), tracker));
		tr_manager.AddRequest(request);   // start retrive peers tracker
    }

	retrieving_peer_ = false;
}

}

