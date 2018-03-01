/*#############################################################################
 * �ļ���   : pps_metadata_retriver.hpp
 * ������   : tom_liu
 * ����ʱ�� : 2013��12��31��
 * �ļ����� : metadata������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEADER_PPS_METADATA_RETRIVER
#define HEADER_PPS_METADATA_RETRIVER

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"

namespace BroadCache
{

struct PpsMetadataDataMsg;

/******************************************************************************
 * ����: PpsMetadataRetriver�����࣬����metadata����ز���
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/31
 *****************************************************************************/
class PpsMetadataRetriver
{
public:
	PpsMetadataRetriver(const TorrentSP& torrent);

	void Start();
	void ReceivedMetadataDataMsg(PpsMetadataDataMsg& msg);
	void TickProc();

	void set_metadata_size(uint64 size) { metadata_size_ = size; } 

private:
	bool GetCandidateConnection();
	void TrySendPieceRequest();
	void TimeoutProc();
	void ConstructPendingPiece();
	bool ConnectionSelectFunc(const PeerConnectionSP& conn);

private:
	struct PendingMetadataPiece
	{
		PendingMetadataPiece() : piece(0), size(0), downloading(false) {}

		uint64 piece;
		uint64 size;
		ptime request_time;
		bool downloading;
	};

private:
	TorrentWP torrent_;

	boost::mutex download_mutex_;

	typedef std::queue<PendingMetadataPiece> MetadataPieceQueue;
	MetadataPieceQueue download_piece_queue_;

	typedef std::queue<PpsPeerConnectionSP> ConnectionQueue;
	ConnectionQueue candidate_conns_;
	
	std::string metadata_cache_;
	uint64 metadata_size_;

	bool retriving_;
};

}

#endif

