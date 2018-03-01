/*#############################################################################
 * 文件名   : peer_connection.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : PeerConnection声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描述: peer连接基类，协议模块需要从此类派生
 * 作者：teck_zhou
 * 时间：2013/09/15
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

	//管理peer下资源信息接口
	uint64 GetUploadSize() { return  traffic_controller_->GetTotalUploadSize(); }
	uint64 GetDownloadSize() { return traffic_controller_->GetTotalDownloadSize(); }
	uint64 GetUploadBandwidth() { return traffic_controller_->GetUploadSpeed(); }
	uint64 GetDownloadBandwidth() { return traffic_controller_->GetDownloadSpeed(); }
	ConnectionID GetConectionID() { return traffic_controller_->peer_conn()->connection_id(); }

	inline void SetFragmentSize(uint64 size) { traffic_controller_->set_fragment_size(size); }

protected: // 协议模块通知上层接口
	void ObtainedInfoHash(const InfoHashSP& info_hash);
	void HandshakeComplete();
	void ReceivedDataResponse(const PeerRequest& request, const char* data);
	void ReceivedDataRequest(const PeerRequest& request);
	void ReceivedChokeMsg(bool choke);
	void ReceivedInterestMsg(bool interest);
	void ReceivedBitfieldMsg(const boost::dynamic_bitset<>& piece_map);  // 设置piece位图
	void ReceivedHaveMsg(uint64 piece_index);

private: // 具体协议实现接口
	virtual void Initialize() = 0;
	virtual void SecondTick() = 0;
	virtual void WriteHaveMsg(int piece_index, char* buf, uint64& len) = 0;
	virtual void WriteDataRequestMsg(const PeerRequest& request, char* buf, uint64& len) = 0;
	virtual void WriteDataResponseMsg(const ReadDataJobSP& job, char* buf, uint64& len) = 0;
	virtual std::vector<MsgRecognizerSP> CreateMsgRecognizers() = 0;

private: // 内部调用函数
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
	boost::scoped_ptr<P2PFsm> fsm_; // 所属状态机

private:
	Session& session_;
	TorrentWP torrent_;

	SocketConnectionSP socket_conn_; // 关联的socket连接

	boost::scoped_ptr<TrafficController> traffic_controller_;
	
	boost::scoped_ptr<DistriDownloadMgr> distri_download_mgr_;

	boost::dynamic_bitset<> piece_map_;  // peer的piece位图

	PeerType peer_type_;

	PeerConnType peer_conn_type_;

	uint32 alive_time_;

	// 上传/下载带宽限制
	uint64 download_bandwidth_limit_;
	uint64 upload_bandwidth_limit_;

	bool started_;

	// 已经有这么长时间没有连接了
	uint32 alive_time_without_traffic_; 

	// 超过这么长时间没有连接，就要将torrent移除
	uint32 max_alive_time_without_traffic_; 

	PktRecombiner pkt_recombiner_;
};


} // namespace

#endif
