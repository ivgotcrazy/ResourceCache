/*#############################################################################
 * �ļ���   : distri_download_msg.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��20��
 * �ļ����� : DistriDownloadMgr����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_DISTRI_DOWNLOAD_MGR
#define HEADER_DISTRI_DOWNLOAD_MGR

#include <set>
#include <vector>
#include <boost/noncopyable.hpp>

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "distri_download_msg.hpp"

namespace BroadCache
{

class PeerConnection;

/******************************************************************************
 * ����: �ֲ�ʽ���ع�����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/20
 *****************************************************************************/
class DistriDownloadMgr : public boost::noncopyable
{
public:
	DistriDownloadMgr(PeerConnection* peer_conn);
	~DistriDownloadMgr();

	void ProcDistriDownloadMsg(const char* buf, uint64 len);

	void OnTick();
	void OnHavePiece(uint32 piece);
	void OnBitfield(const boost::dynamic_bitset<>& piece_map);

private:
	typedef boost::function<void(uint32, char*, uint64*)> MsgConstructor;

private:
	DistriDownloadMsgType GetDistriDownloadMsgType(const char* buf, uint64 len);
	uint32 ParseDistriDownloadMsg(const char* buf, uint64 len);
	
	void OnAssignPieceMsg(uint32 piece);
	void OnAcceptPieceMsg(uint32 piece);
	void OnRejectPieceMsg(uint32 piece);

	void ConstructAssignPieceMsg(uint32 piece, char* buf, uint64* len);
	void ConstructAcceptPieceMsg(uint32 piece, char* buf, uint64* len);
	void ConstructRejectPieceMsg(uint32 piece, char* buf, uint64* len);

	void SendMsgImpl(uint32 piece, MsgConstructor constructor);
	void SendAssignPieceMsg(uint32 piece);
	void SendAcceptPieceMsg(uint32 piece);
	void SendRejectPieceMsg(uint32 piece);

	void TryAssignPieces();
	void AssignPieces(const TorrentSP& torrent);
	void ProcCachedPieces();
	void TimeoutProc();

private:
	PeerConnection* peer_conn_;

	bool support_cache_proxy_;
	bool received_assign_piece_msg_;

	// sponsor����assign-piece��Ϣʱ��torrent���ܻ�δready
	// ��ʱ�����Խ�piece������������torrent ready��ʱ���ٴ���
	std::vector<uint32> cached_pieces_;

	std::set<uint32> assigned_pieces_;

	uint32 after_assigned_piece_time_;
};

}

#endif