/*#############################################################################
 * �ļ���   : peer_connection.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : PeerConnection����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PEER_CONNECTION
#define HEADER_PEER_CONNECTION

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "piece_picker.hpp"
#include "peer_request.hpp"
#include "connection.hpp"
#include "socket_connection.hpp"
#include "traffic_controller.hpp"
#include "pkt_recombiner.hpp"
#include "peer.hpp"

namespace BroadCache
{

class Torrent;
class InfoHash;
class P2PFsm;
class Session;
class Peer;
class ReadDataJob;
class WriteDataJob;
class DiskIoJob;
class TrafficController;
class DistriDownloadMgr;

/******************************************************************************
 * ����: peer���ӻ��࣬Э��ģ����Ҫ�Ӵ�������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class PeerConnection : public boost::noncopyable,
	                   public boost::enable_shared_from_this<PeerConnection>
{
public:
	friend class P2PFsm;
	friend class TrafficController;
	friend class GetBcrmInfo;
	
	PeerConnection(Session& s, 
				   const SocketConnectionSP& conn, 
				   PeerType peer_type, 
		           uint64 download_bandwidth_limit, 
				   uint64 upload_bandwidth_limit);
	virtual ~PeerConnection();

	void Init();
	bool Start();
	bool Stop();
	void OnTick();
	
	void HavePiece(uint64 piece);
	void RetrievedMetadata();
	
	bool IsClosed();
	bool IsInactive() const;
	void InitBitfield(uint64 pieces);
	void AttachTorrent(const TorrentSP& torrent);
	void SetPeerConnType(PeerConnType conn_type) { peer_conn_type_ = conn_type; }

	Session& session() { return session_; }
	TorrentWP torrent() const { return torrent_; }
	SocketConnectionSP socket_connection() { return socket_conn_; }
	ConnectionID connection_id() const { return socket_conn_->connection_id(); }
	boost::dynamic_bitset<> piece_map() const { return piece_map_; }
	PeerType peer_type() const { return peer_type_; }
	PeerConnType peer_conn_type() const { return peer_conn_type_; }

	//����peer����Դ��Ϣ�ӿ�
	uint64 GetUploadSize() { return  traffic_controller_->GetTotalUploadSize(); }
	uint64 GetDownloadSize() { return traffic_controller_->GetTotalDownloadSize(); }
	uint64 GetUploadBandwidth() { return traffic_controller_->GetUploadSpeed(); }
	uint64 GetDownloadBandwidth() { return traffic_controller_->GetDownloadSpeed(); }
	ConnectionID GetConectionID() { return traffic_controller_->peer_conn()->connection_id(); }

	inline void SetFragmentSize(uint64 size) { traffic_controller_->set_fragment_size(size); }

protected: // Э��ģ��֪ͨ�ϲ�ӿ�
	void ObtainedInfoHash(const InfoHashSP& info_hash);
	void HandshakeComplete();
	void ReceivedDataResponse(const PeerRequest& request, const char* data);
	void ReceivedDataRequest(const PeerRequest& request);
	void ReceivedChokeMsg(bool choke);
	void ReceivedInterestMsg(bool interest);
	void ReceivedBitfieldMsg(const boost::dynamic_bitset<>& piece_map);  // ����pieceλͼ
	void ReceivedHaveMsg(uint64 piece_index);

private: // ����Э��ʵ�ֽӿ�
	virtual void Initialize() = 0;
	virtual void SecondTick() = 0;
	virtual void WriteHaveMsg(int piece_index, char* buf, uint64& len) = 0;
	virtual void WriteDataRequestMsg(const PeerRequest& request, char* buf, uint64& len) = 0;
	virtual void WriteDataResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len) = 0;
	virtual std::vector<MsgRecognizerSP> CreateMsgRecognizers() = 0;

private: // �ڲ����ú���
	void ProcessRecvError(ConnectionError error);
	
	void OnReadData(bool ret, const DiskIoJobSP& job);
	void OnWriteData(bool ret, const DiskIoJobSP& job);
	
	void SendHaveMsg(uint64 piece_index);
	
	void SendDataRequest(const PeerRequest& request);
	void SendDataResponse(const PeerRequest& request);
	
	void OnReceiveData(const PeerConnectionWP& weak_conn, 
		               ConnectionError error, 
					   const char* buf, 
					   uint64 bytes_transferred);
	void OnSendData(ConnectionError error, uint64 bytes_transferred);
	
	void AttachToTorrent(const InfoHashSP& info_hash);
	void ProcProtocolMsg(const char* buf, uint64 len);

protected:
	boost::scoped_ptr<P2PFsm> fsm_; // ����״̬��

private:
	Session& session_;
	TorrentWP torrent_;

	SocketConnectionSP socket_conn_; // ������socket����

	boost::scoped_ptr<TrafficController> traffic_controller_;
	
	boost::scoped_ptr<DistriDownloadMgr> distri_download_mgr_;

	boost::dynamic_bitset<> piece_map_;  // peer��pieceλͼ

	PeerType peer_type_;

	PeerConnType peer_conn_type_;

	uint32 alive_time_;

	// �ϴ�/���ش�������
	uint64 download_bandwidth_limit_;
	uint64 upload_bandwidth_limit_;

	bool started_;

	// �Ѿ�����ô��ʱ��û��������
	uint32 alive_time_without_traffic_; 

	// ������ô��ʱ��û�����ӣ���Ҫ��torrent�Ƴ�
	uint32 max_alive_time_without_traffic_; 

	PktRecombiner pkt_recombiner_;
};


} // namespace

#endif
