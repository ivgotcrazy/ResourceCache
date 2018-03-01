/*#############################################################################
 * 文件名   : type_def.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : 宏定义、类型重定义
 * ##########################################################################*/

#ifndef HEADER_TYPE_DEF
#define HEADER_TYPE_DEF

#include "depend.hpp"

namespace BroadCache
{

// shared_ptr封装类型定义, SP: Shared Pointer

class BtCommonMsgRecognizer;
class BtHandshakeMsgRecognizer;
class BtPeerConnection;
class BtTorrent;
class BtTrackerRequest;
class BtUdpTrackerRequest;
class Communicator;
class DiskIoJob;
class FlushWriteCacheJob;
class MetadataRetriver;
class MsgRecognizer;
class Peer;
class PeerConnection;
class PrivateMsgRecognizer;
class ReadDataJob;
class ReadMetadataJob;
class ReadResumeJob;
class Session;
class Torrent;
class TrackerManager;
class TrackerRequest;
class VerifyPieceInfoHashJob;
class WriteDataJob;
class WriteMetadataJob;
class WriteResumeJob;
class PpsTorrent;
class PpsPeerConnection;
class PpsTrackerRequest;
class PpsCommonMsgRecognizer;
class PpsBaseinfoRetriver;
class PpsBaseinfoTrackerRequest;

typedef boost::shared_ptr<BtCommonMsgRecognizer>		BtCommonMsgRecognizerSP;
typedef boost::shared_ptr<BtHandshakeMsgRecognizer>		BtHandshakeMsgRecognizerSP;
typedef boost::shared_ptr<BtPeerConnection>				BtPeerConnectionSP;
typedef boost::shared_ptr<BtTorrent>					BtTorrentSP;
typedef boost::shared_ptr<BtTrackerRequest>				BtTrackerRequestSP;
typedef boost::shared_ptr<BtUdpTrackerRequest>			BtUdpTrackerRequestSP;
typedef boost::shared_ptr<Communicator>					CommunicatorSP;
typedef boost::shared_ptr<DiskIoJob>					DiskIoJobSP;
typedef boost::shared_ptr<WriteMetadataJob>				FlushWriteCacheJobSP;
typedef boost::shared_ptr<MetadataRetriver>				MetadataRetriverSP; 
typedef boost::shared_ptr<MsgRecognizer>				MsgRecognizerSP;
typedef boost::shared_ptr<Peer>							PeerSP;
typedef boost::shared_ptr<PeerConnection>				PeerConnectionSP;
typedef boost::shared_ptr<PrivateMsgRecognizer>			PrivateMsgRecognizerSP;
typedef boost::shared_ptr<ReadDataJob>					ReadDataJobSP;
typedef boost::shared_ptr<ReadMetadataJob>				ReadMetadataJobSP;
typedef boost::shared_ptr<ReadResumeJob>				ReadResumeJobSP;
typedef boost::shared_ptr<Session>						SessionSP;
typedef boost::shared_ptr<Torrent>						TorrentSP;
typedef boost::shared_ptr<TrackerManager>				TrackerManagerSP;
typedef boost::shared_ptr<TrackerRequest>				TrackerRequestSP;
typedef boost::shared_ptr<VerifyPieceInfoHashJob>		VerifyPieceInfoHashJobSP;
typedef boost::shared_ptr<WriteDataJob>					WriteDataJobSP;
typedef boost::shared_ptr<WriteMetadataJob>				WriteMetadataJobSP;
typedef boost::shared_ptr<WriteResumeJob>				WriteResumeJobSP;
typedef boost::shared_ptr<PpsTorrent>					PpsTorrentSP;
typedef boost::shared_ptr<PpsPeerConnection>			PpsPeerConnectionSP;
typedef boost::shared_ptr<PpsTrackerRequest>			PpsTrackerRequestSP;
typedef boost::shared_ptr<PpsCommonMsgRecognizer>		PpsCommonMsgRecognizerSP;
typedef boost::shared_ptr<PpsBaseinfoRetriver>			PpsBaseinfoRetriverSP;
typedef boost::shared_ptr<PpsBaseinfoTrackerRequest>	PpsBaseinfoTrackerRequestSP;


typedef boost::weak_ptr<PeerConnection> PeerConnectionWP;
typedef boost::weak_ptr<Session>		SessionWP;
typedef boost::weak_ptr<TrackerRequest> TrackerRequestWP;
typedef boost::weak_ptr<Torrent>		TorrentWP;

}

#endif
