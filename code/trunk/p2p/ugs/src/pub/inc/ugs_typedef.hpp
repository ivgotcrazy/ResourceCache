/*#############################################################################
 * 文件名   : ugs_typedef.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : Session类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_UGS_TYPEDEF
#define HEADER_UGS_TYPEDEF

#include <boost/shared_ptr.hpp>
#include "bc_typedef.hpp"
#include "endpoint.hpp"

namespace BroadCache
{

//==============================================================================
struct DataEntry
{
	DataEntry() : len(0), buf(0) {}
	DataEntry(const char* data, uint32 length) 
		: len(length), buf(data)
	{
	}

	uint32 len;
	const char* buf;
};

typedef DataEntry Packet;

//==============================shared_ptr声明=================================

class HotAlgorithm;
class PktProcessor;
class Session;
class PeerPool;

typedef boost::shared_ptr<HotAlgorithm> HotAlgorithmSP;
typedef boost::shared_ptr<PktProcessor> PktProcessorSP;
typedef boost::shared_ptr<Session>		SessionSP;
typedef boost::shared_ptr<PeerPool> PeerPoolSP;

//===============================热点资源处理==================================

typedef uint32 msg_seq_t;
typedef uint32 access_times_t;  // 资源访问次数
typedef double access_rank_t;	// 资源访问次数排名
typedef std::string ByteArray;  // 字节数组（一般存放二进制数据）
typedef double hot_value_t;		// 资源热度值

enum DownloadProtocol
{
    PROTOCOL_UNKNOWN,
    PROTOCOL_BITTORRENT,
    PROTOCOL_PPS,
};

struct ResourceId
{
    DownloadProtocol protocol = PROTOCOL_UNKNOWN;
    ByteArray info_hash;
};

// 资源访问记录
struct AccessRecord
{
    ResourceId resource;
    EndPoint peer;
    time_t access_time = 0;
};

typedef EndPoint ServiceNode;

//==============================================================================

}  // namespace BroadCache

#endif  // HEADER_UGS_TYPEDEF
