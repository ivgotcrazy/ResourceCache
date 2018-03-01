/*#############################################################################
 * 文件名   : pps_torrent.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月30日
 * 文件描述 : PpsTorrent类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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

//FR是fast resume的缩写
//fast-resume用bencode编码成一个map时，每一个数据项对应一个字符串表示的key
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
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月11日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 析构函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年09月11日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PpsTorrent::~PpsTorrent()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::Initialize()
{
	baseinfo_retriver_.reset(new PpsBaseinfoRetriver(shared_from_this()));

	metadata_retriver_.reset(new PpsMetadataRetriver(shared_from_this()));

	// 这里读取metadata的目的是希望获取metadata中的piece hash，当前这个信息
	// 只保存在metadata中，当然可能此时还没有metadata，不过没关系，大不了
	// 读取失败，后续从网络获取metadata后，也能将metadata加载到内存。
	//ReadMetadata();  

	// 如果有fast-resume则读取，否则，会触发从网络获取BaseInfo
	ReadFastResume();
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月11日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::TickProc()
{
	if (fastresume_updated_)
	{
		WriteFastResume();
		fastresume_updated_ = false;
	}

	//如果baseinfo还没获取，就调用RetriveBaseInfo
	if (!baseinfo_ready_)
	{	
		baseinfo_retriver_->TickProc();
	}
	
	//如果metadata还没获取到，就调用metadata的定时器
	if (baseinfo_ready_ && metadata_ready_ == false)
	{
		metadata_retriver_->TickProc(); 
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 下载完piece处理
 * 参  数: [in] piece 分片索引
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::HavePieceProc(uint64 piece)
{
	BC_ASSERT(piece < fast_resume_.piece_map.size());

	fast_resume_.piece_map[piece] = true;

	fastresume_updated_ = true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 供BaseinfoRetriver获取Tracker地址接口
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年1月23日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::RetriveTrackerList()
{
	LOG(INFO) << "Retrive Tracker List";

	if (!fast_resume_.trackers.empty())  // fast-resume文件中保存有tracker-list
    { 
    	baseinfo_retriver_->AddTrackerList(fast_resume_.trackers);
    }
    else  // 从UGS获取tracker-list
    {
    	LOG(INFO)<<"Request tracker list for retrive base info ...";	
		
        // 设置请求tracker消息
        PpsRequestTrackerMsg msg;
        msg.set_info_hash(info_hash()->raw_data());

        // 发送请求udp tracker消息
		msg.set_tracker_type(UDP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&PpsTorrent::OnMsg, this, _1));
    }
}

/*------------------------------------------------------------------------------
 * 描  述: 获取更多的peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2014.1.3
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::RetrivePeers()
{
	if (!baseinfo_ready_) // baseinfo没有获取就直接退出
		return; 
	
    if (retrieving_peer_)  // 正在获取peer-list
        return ;

	LOG(INFO) << "Policy call retrive peer";

    retrieving_peer_ = true;  // 设置正在获取peer-list
	if (!fast_resume_.trackers.empty())  // fast-resume文件中保存有tracker-list
    { 
    	RetrievePeersFromTrackers();
    }
    else  // 从UGS获取tracker-list
    {
    	LOG(INFO)<<"Request tracker list";	
        // 设置请求tracker消息
        PpsRequestTrackerMsg msg;
        msg.set_info_hash(info_hash()->raw_data());

        // 发送请求udp tracker消息
		msg.set_tracker_type(UDP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&PpsTorrent::OnMsg, this, _1));
    }
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取已经缓存资源的RCS
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 获取本地缓存代理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 获取外部缓存代理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 收到来自UGS发过来的消息
 * 参  数: [in] msg UGS发过来的消息
 * 返回值:
 * 修  改:
 *   时间 2013.10.11
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
void PpsTorrent::OnMsg(const PbMessage& msg)
{
    msg_dispatcher_.Dispatch(msg);
}

/*------------------------------------------------------------------------------
 * 描  述: 处理ugs回复的tracker消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 tom_liu
 *   描述 创建
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
	
	if (!baseinfo_ready_) //如果没有获取baseinfo 先获取baseinfo
		baseinfo_retriver_->AddTrackerList(fast_resume_.trackers);
	else
    	RetrievePeersFromTrackers();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取Metadata信息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月26日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::TryRetriveMetadata()
{
	LOG(INFO) << "Try Retrive Metadata";

	LOG(INFO) << "Metadata size : " << fast_resume_.metadata_size;

	//Todo 对 fastresume获取进行判断
	metadata_retriver_->set_metadata_size(fast_resume_.metadata_size);
		
	metadata_retriver_->Start();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取Metadata信息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月26日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::TryRetriveBaseInfo()
{
	set_retrieved_baseInfo(false);
	baseinfo_retriver_->Start();
}

/*------------------------------------------------------------------------------
 * 描  述: 将fast-resume信息写入bencode对象中
 * 参  数:
 * 返回值: 
 * 修  改:
 *   时间 2013.12.30
 *   作者 tom_liu
 *   描述 创建
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
	entry[kFrPieceMap]     = BencodeEntry(ToString(GetReversedBitset(fast_resume_.piece_map)));  // 将piece_map转换成字符串
	
	auto& udp_trackers = entry[kFrTrackerList].List();
	for (const EndPoint& tracker : fast_resume_.trackers)
	{
		udp_trackers.push_back(BencodeEntry(to_string(tracker)));
	}
}

/*------------------------------------------------------------------------------
 * 描  述: 从bencode对象中读取fast-resume信息
 * 参  数:
 * 返回值: 
 * 修  改:
 *   时间 2013.12.30
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 更新torrent信息
 * 参  数: 
 * 返回值: 
 * 修  改: 
 *   时间 2013年11月07日
 *   作者 tom_liu
 *   描述 创建
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
	
	// 通知Torrent
	SetTorrentInfo(torrent_info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 添加Baseinfo的接口
 * 参  数: 
 * 返回值: 
 * 修  改: 
 *   时间 2013年12月30日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::AddBaseInfo(PpsBaseInfoMsg& msg)
{
	if (fastresume_updated_) return; 
	
	LOG(INFO) << " Add base info ";

	set_retrieved_baseInfo(true);

	//更新fastresume中的信息
	RefreshFastResumeFromBaseInfo(msg);

	//更新torrentinfo中的信息
	UpdateTorrentInfo(FROM_BASEINFO);	//ugs proto文件中要添加这个

	//获取peer-list
	RetrievePeersFromTrackers();

	//metadata_retriver在 torrentinfo更新后启动
	TryRetriveMetadata();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取baseinfo后更新fast-resume
 * 参  数: 
 * 返回值: 
 * 修  改: 
 *   时间 2013年10月19日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::RefreshFastResumeFromBaseInfo(PpsBaseInfoMsg& base_info)
{
	BC_ASSERT(!fastresume_updated_);

	BencodeEntry entry;

	// 设置fast-resume
	//fast_resume_.name       = base_info.torrent_name;
	fast_resume_.info_hash  = info_hash();
	fast_resume_.total_size = base_info.file_size;
	fast_resume_.piece_size = CalcPpsPieceSize(base_info.file_size, base_info.piece_count);
	fast_resume_.block_size = PPS_BLOCK_SIZE;  //pps的block大小为1024
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
	entry[kFrPieceMap]    = BencodeEntry(ToString(fast_resume_.piece_map));  // 将piece_map转换成字符串
	entry[kFrTrackerList];   // 创建一个空的BencodeEntry

	fastresume_updated_ = true;
}



/*-----------------------------------------------------------------------------
 * 描  述: 从网络获取到metadata后的处理
 * 参  数: [in] metadata 数据
 * 返回值: 
 * 修  改: 
 *   时间 2013年10月19日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsTorrent::RetrivedMetadata(const std::string& metadata)
{
	//只进行metadata长度的判断
	if (!CheckMetadata(metadata)) return;
	
	WriteMetadata(metadata);

	metadata_ready_ = true;
}

/*------------------------------------------------------------------------------
 * 描  述: 将fast-resume对应的bencode编码数据写到文件中
 * 参  数: 
 * 返回值: 写入是否成功
 * 修  改:
 *   时间 2013.09.17
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool PpsTorrent::WriteFastResume()
{
	BencodeEntry entry;
    SetFastResumeToBencode(entry); //设置fast-resume对应的bencode编码数据

	std::string entry_str = Bencode(entry);
     
    io_oper_manager()->AsyncWriteFastResume(
		boost::bind(&PpsTorrent::OnFastResumeWritten, this, _1, _2), entry_str);

    return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 读取fast-resume数据
 * 参  数:
 * 返回值: 
 * 修  改:
 *   时间 2013.09.17
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
void PpsTorrent::ReadFastResume()
{
    io_oper_manager()->AsyncReadFastResume(
		boost::bind(&PpsTorrent::OnFastResumeRead, this, _1, _2));
}

/*------------------------------------------------------------------------------
 * 描  述: 从文件中读取metadata信息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.06
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::ReadMetadata()
{
    io_oper_manager()->AsyncReadMetadata(
		boost::bind(&PpsTorrent::OnMetadataRead, this, _1, _2));
}


/*------------------------------------------------------------------------------
 * 描  述: fast-resume文件写完成后的回调函数
 * 参  数: [in] ok fast-resume文件写是否成功
 *         [in] DiskIoJob 是哪一个DiskIoJob完成此次写操作的，未使用
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
void PpsTorrent::OnFastResumeWritten(bool ok, const DiskIoJobSP& job)
{
	WriteResumeJobSP write_job = SP_CAST(WriteResumeJob, job);

    if (!ok) //写fast-resume失败
    {
        LOG(ERROR) << "Fail to write fast resume.\n";
    }
}

bool PpsTorrent::CheckMetadata(const std::string& metadata)
{
	LOG(INFO) << "metadata size: " << metadata.size() << "fast resume size: " 
			<< fast_resume_.metadata_size;
		
	if (metadata.size() != fast_resume_.metadata_size) return false;

	//设置metadta正常
	metadata_ready_ = true;

	return true;
}


/*------------------------------------------------------------------------------
 * 描  述: metadata读取完成后的回调函数
 * 参  数: [in] ok metadata读取是否成功
 *         [in] job 完成读取metadata对象
 * 返回值:
 * 修  改:
 *   时间 2013.11.06
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void PpsTorrent::OnMetadataRead(bool ok, const DiskIoJobSP& job)
{
	ReadMetadataJobSP read_job = SP_CAST(ReadMetadataJob, job);

	LOG(INFO) << "On metadata read";
	
	// 如果读取失败，则从网络获取metadata
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
 * 描  述: 校验fast-resume数据是否合法
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 tom_liu
 *   描述 创建
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

	// 校验资源大小
	if (fast_resume_.total_size > fast_resume_.piece_size * fast_resume_.piece_map.size())
	{
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: 校验资源piece-map
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
void PpsTorrent::VerifyResource()
{
	uint64 piece_num = fast_resume_.piece_map.size();
	uint64 common_piece_size = fast_resume_.piece_size;

	// 由于前面已经校验过FastResume，这里相减不会翻转
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
 * 描  述: fast-resume文件读取完成后的回调函数
 * 参  数: [in] ok fast-resume文件读取是否成功
 *         [in] DiskIoJob 是哪一个DiskIoJob完成此次读取操作的，未使用
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
void PpsTorrent::OnFastResumeRead(bool ok, const DiskIoJobSP& job)
{
	ReadResumeJobSP read_job = SP_CAST(ReadResumeJob, job);

	// 如果读取失败，则从网络获取metadata
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

	//检验metadata是否有获取
	ReadMetadata();  	

	// 解析fast-resume
	BencodeEntry entry;
	entry = Bdecode(read_job->fast_resume); 
	GetFastResumeFromBencode(entry);

	// 校验fast-resume信息，如果不合法则从网络获取	
	if (!CheckFastResume())
	{
		LOG(ERROR) << "Invalid fast-resume data";
		TryRetriveBaseInfo();  //此处在Pps是获取baseinfo
		return;
	}

	// 通知torrent一切OK
	UpdateTorrentInfo(FROM_FASTRESUME);

	// fast-resume读取成功且内容合法，则需要校验资源数据
	VerifyResource();
}

/*------------------------------------------------------------------------------
 * 描  述: metadata写入文件
 * 参  数: [in] buf 数据
 *         [in] len 长度
 * 返回值:
 * 修  改:
 *   时间 2014年1月3日
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
void PpsTorrent::WriteMetadata(const std::string& metadata)
{
	io_oper_manager()->AsyncWriteMetadata(
		boost::bind(&PpsTorrent::OnMetadataWritten, this, _1, _2), metadata);
}

/*------------------------------------------------------------------------------
 * 描  述: metadata文件写入完成后的回调函数
 * 参  数: [in] ok metadata文件写入是否成功
 *         [in] job 是哪一个DiskIoJob完成此次读取操作的
 * 返回值:
 * 修  改:
 *   时间 2013年10月19日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 校验piece的info-hash是否正确
 * 参  数: [in] piece 待校验piece的索引
 *         [in] data piece对应的数据指针
 *         [in] length piece对应的数据长度
 * 返回值: info-hash是否正确
 * 修  改:
 *   时间 2013.11.06
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool PpsTorrent::VerifyPiece(uint64 piece, const char* data, uint32 length)
{
    return true;  //pps不做piece的校验，不解析metadata
}

/*------------------------------------------------------------------------------
 * 描  述: 从tracker服务器获取peer-list 
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2014.1.3
 *   作者 tom_liu
 *   描述 创建
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

