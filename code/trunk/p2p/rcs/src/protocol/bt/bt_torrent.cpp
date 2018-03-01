/*#############################################################################
 * 文件名   : bt_torrent.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月13日
 * 文件描述 : BtTorrent类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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

//FR是fast resume的缩写
//fast-resume用bencode编码成一个map时，每一个数据项对应一个字符串表示的key
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
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月11日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 析构函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年09月11日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
BtTorrent::~BtTorrent()
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtTorrent::Initialize()
{
	metadata_retriver_.reset(new BtMetadataRetriver(shared_from_this()));

	// 这里读取metadata的目的是希望获取metadata中的piece hash，当前这个信息
	// 只保存在metadata中，当然可能此时还没有metadata，不过没关系，大不了
	// 读取失败，后续从网络获取metadata后，也能将metadata加载到内存。
	ReadMetadata();

	// 如果有fast-resume则读取，否则，会触发从网络获取metadata
	ReadFastResume();
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月11日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtTorrent::TickProc()
{
	if (fastresume_updated_)
	{
		WriteFastResume();
		fastresume_updated_ = false;
	}

	//如果metadata还没获取到，就调用metadata的定时器
	if (is_ready() == false)
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
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtTorrent::HavePieceProc(uint64 piece)
{
	BC_ASSERT(piece < fast_resume_.piece_map.size());

	fast_resume_.piece_map[piece] = true;

	fastresume_updated_ = true;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取更多的peer
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.10.16
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void BtTorrent::RetrivePeers()
{
    if (retrieving_peer_)  // 正在获取peer-list
        return ;

    retrieving_peer_ = true;  // 设置正在获取peer-list
	if (!fast_resume_.http_trackers.empty())  // fast-resume文件中保存有tracker-list
    { 
    	RetrievePeersFromTrackers();
    }
    else  // 从UGS获取tracker-list
    {
    	LOG(INFO)<<"Request tracker list...";	
        // 设置请求tracker消息
        BtRequestTrackerMsg msg;
        msg.set_info_hash(info_hash()->raw_data());

        // 发送请求http tracker消息
		msg.set_tracker_type(HTTP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&BtTorrent::OnMsg, this, _1));

        // 发送请求udp tracker消息
		msg.set_tracker_type(UDP_TRACKER);
        Communicator::GetInstance().SendMsg(msg, boost::bind(&BtTorrent::OnMsg, this, _1));
    }
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取已经缓存资源的RCS
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 获取本地缓存代理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 获取外部缓存代理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月21日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 收到来自UGS发过来的消息
 * 参  数: [in] msg UGS发过来的消息
 * 返回值:
 * 修  改:
 *   时间 2013.10.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtTorrent::OnMsg(const PbMessage& msg)
{
    msg_dispatcher_.Dispatch(msg);
}

/*------------------------------------------------------------------------------
 * 描  述: 处理ugs回复的tracker消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 处理ugs回复的rcs消息
 * 参  数: [in] msg 消息对象
 * 返回值:
 * 修  改:
 *   时间 2013.11.23
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 获取Metadata信息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月26日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtTorrent::TryRetriveMetadata()
{
	metadata_retriver_->Start();
}

/*------------------------------------------------------------------------------
 * 描  述: 将fast-resume信息写入bencode对象中
 * 参  数:
 * 返回值: 
 * 修  改:
 *   时间 2013.10.16
 *   作者 rosan
 *   描述 创建
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
	entry[kFrPieceMap]    = BencodeEntry(ToString(GetReversedBitset(fast_resume_.piece_map)));  // 将piece_map转换成字符串

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
 * 描  述: 从bencode对象中读取fast-resume信息
 * 参  数:
 * 返回值: 
 * 修  改:
 *   时间 2013.10.16
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 解析metadata
 * 参  数: [in] metadata 数据
 * 返回值: 成功/失败
 * 修  改: 
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 *   时间 2013年10月8日
 *   作者 vicent_pan
 *   枋?实现
 ----------------------------------------------------------------------------*/
bool BtTorrent::ParseMetadata(const std::string& metadata)
{
	std::string file_path;	

	// 通过info_hash判断metadata_cache是否正确
	if (info_hash()->raw_data() != GetSha1(metadata.c_str(), metadata.size()))
	{
		LOG(ERROR) << "Invalid metadata" ;
		return false;
	}
	else
	{
		bt_metadata_.info_hash = info_hash();
	}

	LazyEntry bde_metadata;      // 创建一个空的lazyEntry作为存储解码后的数据。
	const char * data = metadata.c_str(); 
	if (LazyBdecode(data, data + (metadata.size() - 1), bde_metadata)) //LazyBdecode解码。
	{
		LOG(ERROR)<< "Fail to decode metadata cache";
		return false;
	}	  

	uint64 file_single_size = 0;   // 单个文件的大小
	uint64 file_total_size = 0;    // 资源中所用文件的总大小

	if (const LazyEntry*file_bde = bde_metadata.DictFindList("files")) // 多文件的bendcode
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
	else  // 单文件的bendcode
	{	
		file_single_size = bde_metadata.DictFindIntValue("length");

		// 对于单文件来说，文件名就是资源名
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

	//TODO: 由于要支持metadata协议，这里暂时先保存到torrent
	// 后续将metadata的管理独立到一个类中
	metadata_origin_ = metadata;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 更新torrent信息
 * 参  数: 
 * 返回值: 
 * 修  改: 
 *   时间 2013年11月07日
 *   作者 teck_zhou
 *   描述 创建
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
	
	// 通知Torrent
	SetTorrentInfo(torrent_info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 从网络获取到metadata后的处理
 * 参  数: [in] metadata 数据
 * 返回值: 
 * 修  改: 
 *   时间 2013年10月19日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 获取metadata后更新fast-resume
 * 参  数: 
 * 返回值: 
 * 修  改: 
 *   时间 2013年10月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void BtTorrent::RefreshFastResumeFromMetadata()
{
	BC_ASSERT(!fastresume_updated_);

	BencodeEntry entry;

	// 设置fast-resume
	fast_resume_.name       = bt_metadata_.torrent_name;
	fast_resume_.info_hash  = bt_metadata_.info_hash;
	fast_resume_.total_size = bt_metadata_.total_size;
	fast_resume_.piece_size = bt_metadata_.piece_size;
	fast_resume_.block_size = BT_BLOCK_SIZE; //TODO: 关于block大小的确定，后续还要继续看下
	fast_resume_.piece_map.resize(CALC_FULL_MULT(bt_metadata_.total_size, bt_metadata_.piece_size));

	entry[kFrDescription] = BencodeEntry(BT_FAST_RESUME_DESC);
	entry[kFrVersion]     = BencodeEntry(BT_FAST_RESUME_VERSION);
	entry[kFrProtocol]    = BencodeEntry(BT_FAST_RESUME_PROTOCOL);
	entry[kFrName]        = BencodeEntry(fast_resume_.name);
	entry[kFrInfoHash]    = BencodeEntry(fast_resume_.info_hash->to_string());
	entry[kFrTotalSize]   = BencodeEntry(fast_resume_.total_size);
	entry[kFrPieceSize]   = BencodeEntry(fast_resume_.piece_size);
	entry[kFrBlockSize]   = BencodeEntry(fast_resume_.block_size);
	entry[kFrPieceMap]    = BencodeEntry(ToString(fast_resume_.piece_map));  // 将piece_map转换成字符串
	entry[kFrHttpTrackerList];   // 创建一个空的BencodeEntry
	entry[kFrUdpTrackerList];   // 创建一个空的BencodeEntry

	fastresume_updated_ = true;
}

/*------------------------------------------------------------------------------
 * 描  述: 将fast-resume对应的bencode编码数据写到文件中
 * 参  数: 
 * 返回值: 写入是否成功
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtTorrent::WriteFastResume()
{
	BencodeEntry entry;
    SetFastResumeToBencode(entry); //设置fast-resume对应的bencode编码数据

	std::string entry_str = Bencode(entry);
     
    io_oper_manager()->AsyncWriteFastResume(
		boost::bind(&BtTorrent::OnFastResumeWritten, this, _1, _2), entry_str);

    return true;
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
void BtTorrent::ReadMetadata()
{
    io_oper_manager()->AsyncReadMetadata(
		boost::bind(&BtTorrent::OnMetadataRead, this, _1, _2));
}

/*------------------------------------------------------------------------------
 * 描  述: 读取fast-resume数据
 * 参  数:
 * 返回值: 
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtTorrent::ReadFastResume()
{
    io_oper_manager()->AsyncReadFastResume(
		boost::bind(&BtTorrent::OnFastResumeRead, this, _1, _2));
}

/*------------------------------------------------------------------------------
 * 描  述: fast-resume文件写完成后的回调函数
 * 参  数: [in] ok fast-resume文件写是否成功
 *         [in] DiskIoJob 是哪一个DiskIoJob完成此次写操作的，未使用
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtTorrent::OnFastResumeWritten(bool ok, const DiskIoJobSP& job)
{
	WriteResumeJobSP write_job = SP_CAST(WriteResumeJob, job);

    if (!ok) //写fast-resume失败
    {
        LOG(ERROR) << "Fail to write fast resume.\n";
    }
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
 * 描  述: 校验fast-resume数据是否合法
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 teck_zhou
 *   描述 创建
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
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtTorrent::VerifyResource()
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
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtTorrent::OnFastResumeRead(bool ok, const DiskIoJobSP& job)
{
	ReadResumeJobSP read_job = SP_CAST(ReadResumeJob, job);

	// 如果读取失败，则从网络获取metadata
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

	// 解析fast-resume
	BencodeEntry entry;
	entry = Bdecode(read_job->fast_resume); 
	GetFastResumeFromBencode(entry);

	// 校验fast-resume信息，如果不合法则从网络获取	
	if (!CheckFastResume())
	{
		LOG(ERROR) << "Invalid fast-resume data";
		TryRetriveMetadata();
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
 *   时间 2013年10月19日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
void BtTorrent::WriteMetadata(const std::string& metadata)
{
	io_oper_manager()->AsyncWriteMetadata(
		boost::bind(&BtTorrent::OnMetadataWritten, this, _1, _2), metadata);
}

/*------------------------------------------------------------------------------
 * 描  述: metadata文件写入完成后的回调函数
 * 参  数: [in] ok metadata文件写入是否成功
 *         [in] job 是哪一个DiskIoJob完成此次读取操作的
 * 返回值:
 * 修  改:
 *   时间 2013年10月19日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 校验piece的info-hash是否正确
 * 参  数: [in] piece 待校验piece的索引
 *         [in] data piece对应的数据指针
 *         [in] length piece对应的数据长度
 * 返回值: info-hash是否正确
 * 修  改:
 *   时间 2013.11.06
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool BtTorrent::VerifyPiece(uint64 piece, const char* data, uint32 length)
{
    return bt_metadata_.piece_hash[piece]->raw_data() == GetSha1(data, length);
}

/*------------------------------------------------------------------------------
 * 描  述: 从tracker服务器获取peer-list 
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.13
 *   作者 rosan
 *   描述 创建
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
