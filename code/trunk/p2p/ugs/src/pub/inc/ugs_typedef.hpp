/*#############################################################################
 * �ļ���   : ugs_typedef.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : Session������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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

//==============================shared_ptr����=================================

class HotAlgorithm;
class PktProcessor;
class Session;
class PeerPool;

typedef boost::shared_ptr<HotAlgorithm> HotAlgorithmSP;
typedef boost::shared_ptr<PktProcessor> PktProcessorSP;
typedef boost::shared_ptr<Session>		SessionSP;
typedef boost::shared_ptr<PeerPool> PeerPoolSP;

//===============================�ȵ���Դ����==================================

typedef uint32 msg_seq_t;
typedef uint32 access_times_t;  // ��Դ���ʴ���
typedef double access_rank_t;	// ��Դ���ʴ�������
typedef std::string ByteArray;  // �ֽ����飨һ���Ŷ��������ݣ�
typedef double hot_value_t;		// ��Դ�ȶ�ֵ

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

// ��Դ���ʼ�¼
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
