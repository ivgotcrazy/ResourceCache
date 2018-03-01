/*#############################################################################
 * �ļ���   : peer_connection.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : PeerConnectionʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "peer_connection.hpp"
#include "p2p_fsm.hpp"
#include "info_hash.hpp"
#include "mem_buffer_pool.hpp"
#include "bc_assert.hpp"
#include "torrent.hpp"
#include "io_oper_manager.hpp"
#include "piece_picker.hpp"
#include "peer_request.hpp"
#include "session.hpp"
#include "socket_connection.hpp"
#include "policy.hpp"
#include "rcs_util.hpp"
#include "rcs_config_parser.hpp"
#include "bc_util.hpp"
#include "distri_download_msg_recognizer.hpp"
#include "distri_download_mgr.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PeerConnection::PeerConnection(Session& s, const SocketConnectionSP& conn, 
	PeerType peer_type, uint64 download_bandwidth_limit, uint64 upload_bandwidth_limit)
    : session_(s)
    , socket_conn_(conn)
	, peer_type_(peer_type)
	, peer_conn_type_(PEER_CONN_COMMON)
	, alive_time_(0)
	, download_bandwidth_limit_(download_bandwidth_limit)
	, upload_bandwidth_limit_(upload_bandwidth_limit)
	, started_(false)
	, alive_time_without_traffic_(0)	
	, max_alive_time_without_traffic_(600)
{
	ConnectionType conn_type = socket_conn_->connection_type();

	// ���ڱ���������˵�����Ӵ���ʱ��peer����ֻ��ʶ��Ϊinner peer����outer peer
	// ��ʱ���޷�ȷ�ϴ����ӵ����ǲ���proxy����Ȼ�������Ӳ�������sponsor������
	// �������������Ӵ���ʱֻ����common�����ֱ��ʹ��Ĭ��ֵ���ɣ����ô���
	if ((conn_type == CONN_ACTIVE)
		&& (peer_type_ == PEER_INNER_PROXY || peer_type_ == PEER_OUTER_PROXY))
	{
		peer_conn_type_ = PEER_CONN_SPONSOR;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PeerConnection::~PeerConnection()
{
	LOG(INFO) << ">>> Destructing peer connection | " << connection_id();

	Stop();

	socket_conn_->Close();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ʼ������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::Init()
{
	traffic_controller_.reset(new TrafficController(this, 
		download_bandwidth_limit_, upload_bandwidth_limit_));

	distri_download_mgr_.reset(new DistriDownloadMgr(this));

	fsm_.reset(new P2PFsm(this, traffic_controller_.get()));

	// ���ñ��Ľ��մ�����
	PeerConnectionWP weak_conn = shared_from_this();
	socket_conn_->set_recv_handler(boost::bind(
		&PeerConnection::OnReceiveData, this, weak_conn, _1, _2, _3));

	// ע��ֲ�ʽ����Э����Ϣʶ����
	MsgRecognizerSP recognizer(new DistriDownloadMsgRecognizer());
	pkt_recombiner_.RegisterMsgRecognizer(recognizer, boost::bind(
		&DistriDownloadMgr::ProcDistriDownloadMsg, distri_download_mgr_.get(), _1, _2));

	// ע��Э����Ϣʶ����
	std::vector<MsgRecognizerSP> recognizers = CreateMsgRecognizers();
	BC_ASSERT(!recognizers.empty());
	for (MsgRecognizerSP& recognizer : recognizers)
	{
		pkt_recombiner_.RegisterMsgRecognizer(recognizer, 
			boost::bind(&PeerConnection::ProcProtocolMsg, this, _1, _2));
	}

	GET_RCS_CONFIG_INT("global.peerconnection.max-alive-time-without-traffic", max_alive_time_without_traffic_);

	Initialize(); // ����Э��ģ���ʼ��
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ľ��մ���
 * ��  ��: error ������
 *         buf ��������
 *         bytes_transferred ���ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::OnReceiveData(const PeerConnectionWP& weak_conn, 
								   ConnectionError error, 
								   const char* buf, 
								   uint64 bytes_transferred)
{
	PeerConnectionSP conn = weak_conn.lock();
	if (!conn) return;

	if (error != CONN_ERR_SUCCESS)
	{
		ProcessRecvError(error);
		return;
	}

	alive_time_without_traffic_ = 0;

	if (conn->connection_id().protocol == CONN_PROTOCOL_TCP)
	{
		pkt_recombiner_.AppendData(buf, bytes_transferred);
	}
	else
	{	
		//�����udpЭ�鲻���Ƿְ��������ճ��
		pkt_recombiner_.AppendUdpData(buf, bytes_transferred);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: Э�鱨�Ĵ���
 * ��  ��: [in] buf ����
 *         [in] len ���ĳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��18��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ProcProtocolMsg(const char* buf, uint64 len)
{
    MessageEntry fsm_msg(buf, len);
	fsm_->ProcessMsg(FSM_MSG_PKT, static_cast<void*>(&fsm_msg));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PeerConnection::Start()
{
	if (!fsm_->StartFsm())
	{
		LOG(ERROR) << "Fail to start FSM";
		return false;
	}

	started_ = true;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PeerConnection::Stop()
{
	started_ = false;

	fsm_->ProcessMsg(FSM_MSG_CLOSE, 0);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �붨ʱ��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::OnTick()
{
	if (!started_) return;

	traffic_controller_->OnTick();

	distri_download_mgr_->OnTick();

	fsm_->ProcessMsg(FSM_MSG_TICK, 0);

	alive_time_++; //��ʱ

	alive_time_without_traffic_++;  //δͨ��ʱ���ʱ

	SecondTick(); // ����Э��ģ�鴦��

}

/*-----------------------------------------------------------------------------
 * ��  ��: ����torrent
 * ��  ��: [in] torrent ����torrent
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��12��03��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::AttachTorrent(const TorrentSP& torrent) 
{ 
	torrent_ = torrent;

	if (torrent->is_ready() && piece_map_.empty())
	{
		InitBitfield(torrent->GetPieceNum());
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������Session�����ģ�����ʱ���޷��������ģ����Ҳ���޷�ȷ��������
 *         ����Torrent����Torrent������ɺ�Ҫ�����Ӱ󶨵�������Torrent
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::AttachToTorrent(const InfoHashSP& info_hash)
{
	BC_ASSERT(!torrent_.lock());

	// ���ȴ�Session�в���Torrent
	TorrentSP torrent = session_.FindTorrent(info_hash);
	if (!torrent)
	{
		torrent = session_.NewTorrent(info_hash);
		if (!torrent)
		{
			LOG(ERROR) << "Fail to new torrent | " << info_hash;
			return;
		}
	}

	// ���ڱ������ӣ�����������torrent�����Ӿͽ���torrent������
	torrent->policy()->IncommingPeerConnecton(shared_from_this());
	session_.RemovePeerConnection(connection_id());

	AttachTorrent(torrent); // torrent�������
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ӵ�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ProcessRecvError(ConnectionError error)
{
	socket_conn_->Close();

	fsm_->ProcessMsg(FSM_MSG_CLOSE, 0);
}

/*-----------------------------------------------------------------------------
 * ��  ��: Torrent��ȡ��Metadata֪ͨ����PeerConnection
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::RetrievedMetadata()
{
	fsm_->ProcessMsg(FSM_MSG_GET_MEATADATA, 0);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ɺ���Ҫ������״̬ת�Ƶ�����״̬
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::HandshakeComplete()
{
	fsm_->ProcessMsg(FSM_MSG_HANDSHAKED, 0);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ����ݿ�Ĵ���
 * ��  ��: [in] request ����������piece��Ϣת������
 *         [in] data �����Ӧ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedDataResponse(const PeerRequest& request, const char* data)
{
	BC_ASSERT(data);

	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;
	
	torrent->io_oper_manager()->AsyncWrite(request, const_cast<char*>(data),
		boost::bind(&PeerConnection::OnWriteData, shared_from_this(), _1, _2));
}

/*-----------------------------------------------------------------------------
 * ��  ��: blockд����̺�Ļص�����
 * ��  ��: [in] ret д����
 *         [in] job д����Job
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::OnWriteData(bool ret, const DiskIoJobSP& job)
{
	WriteDataJobSP write_job = SP_CAST(WriteDataJob, job);

	// ����д�뵽�����֪ͨTrafficController 
	traffic_controller_->ReceivedDataResponse(ToPeerRequest(*write_job));

	if (ret)
	{
		LOG(INFO) << "Success to write data to disk | " << *write_job;
	}
	else
	{
		LOG(ERROR) << "Fail to write data to disk | " << *write_job;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������������
 * ��  ��: [in] request ��������
 * ����?
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ��?teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::SendDataRequest(const PeerRequest& request)
{
	LOG(INFO) << "Send data request msg | " << request;

	SendBuffer send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);
	
	WriteDataRequestMsg(request, send_buf.buf, send_buf.len);

	socket_conn_->SendData(send_buf);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������������Ӧ
 * ��  ��: [in] request peer����������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::SendDataResponse(const PeerRequest& request)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	LOG(INFO) << "Send data response msg | " << request;

	// �������ݿռ䣬��ǰ�涨peerһ�λ�ȡ�����ݲ��ܳ���һ��block
	char* buf = session_.mem_buf_pool().AllocDataBuffer(DATA_BUF_BLOCK);

	torrent->io_oper_manager()->AsyncRead(request, buf,                           
		boost::bind(&PeerConnection::OnReadData, shared_from_this(), _1, _2));
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ӵ��̶�ȡ���ݺ�Ļص�����
 * ��  ��: [in] ret ���
 *         [in] job ������Job
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��16��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::OnReadData(bool ret, const DiskIoJobSP& job)
{
	ReadDataJobSP read_job = SP_CAST(ReadDataJob, job);

	if (ret)
	{
		// ���뷢�ͻ���
		SendBuffer send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_DATA);
		
		// ������Ӧ���������Ҫ֪ͨTrafficController�����Կ����ϴ�����
		send_buf.send_buffer_handler = boost::bind(&TrafficController::OnSendDataResponse, 
			traffic_controller_.get(), _1, ToPeerRequest(*read_job));

		// ����Э��ģ�鹹����Ӧ��Ϣ
		WriteDataResponseMsg(read_job, send_buf.buf, send_buf.len);

		// ������Ϣ
		socket_conn_->SendData(send_buf);

		LOG(INFO) << "Send response to peer | " << *(socket_conn_.get());
	}
	else
	{
		LOG(ERROR) << "Fail to read data from disk | " << *read_job;
	}

	// �ͷ�������ڴ�
	session_.mem_buf_pool().FreeDataBuffer(DATA_BUF_BLOCK, read_job->buf);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ��Զ�bitfiled��������pieceλͼ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedBitfieldMsg(const boost::dynamic_bitset<>& piece_map)
{	
	piece_map_ = piece_map;

	distri_download_mgr_->OnBitfield(piece_map);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ�Choke/Unchoke��Ϣ
 * ��  ��: [in] choke �Ƿ�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedChokeMsg(bool choke)
{
	if (choke)
	{
		fsm_->ProcessMsg(FSM_MSG_CHOKED, 0);
	}
	else
	{
		fsm_->ProcessMsg(FSM_MSG_UNCHOKED, 0);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ�Interested/Uninterested��Ϣ
 * ��  ��: [in] interest �Ƿ����Ȥ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��28��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedInterestMsg(bool interest)
{
	if (interest)
	{
		fsm_->ProcessMsg(FSM_MSG_INTEREST, 0);
	}
	else
	{
		fsm_->ProcessMsg(FSM_MSG_NONINTEREST, 0);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ��InfoHash
 * ��  ��: [in] info_hash ��ԴInfoHash
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ObtainedInfoHash(const InfoHashSP& info_hash)
{
	// ��������Ҳ���յ�֪ͨ�����账��
	TorrentSP torrent = torrent_.lock();
	if (torrent) return;

	AttachToTorrent(info_hash);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ���������
 * ��  ��: [in] request ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��16��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedDataRequest(const PeerRequest& request)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return;

	// �����piece�Ƿ��Ѿ�����
	if (!torrent->piece_picker()->IsPieceFinished(request.piece))
	{
		LOG(WARNING) << "Request unfinished piece(" << request.piece << ")";
		return;
	}

	// ����ĳ����Ƿ�Ϸ�
	if (request.start + request.length > torrent->GetCommonPieceSize())
	{
		LOG(WARNING) << "Invalid data request | " << request;
		return;
	}

	// ����TrafficControllerȥ����
	traffic_controller_->ReceivedDataRequest(request);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �յ�have-piece��Ϣ����
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::ReceivedHaveMsg(uint64 piece)
{
	BC_ASSERT(piece < piece_map_.size());

	piece_map_[piece] = true;

	distri_download_mgr_->OnHavePiece(static_cast<uint32>(piece));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֪ͨ����peer�Լ��Ѿ���ȡ��һ��piece
 * ��  ��: [in] piece ��Ƭ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PeerConnection::HavePiece(uint64 piece)
{
    SendHaveMsg(piece);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ʼ���Է�peer��bitfield����
 *         ����ڶԷ�û�з���bitfield����(��ʱ�Է�peerû�������κ�piece)
 * ��  ��: [in] pieces pieces����Ŀ,Ҳ��bitfield�Ĵ�С
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.28
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void PeerConnection::InitBitfield(uint64 pieces)
{
    if (piece_map_.empty())
    {
        piece_map_.resize(pieces);
    }
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж������Ƿ�ر�
 * ��  ��: 
 * ����ֵ: �Ƿ�ر�
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool PeerConnection::IsClosed()
{
	return fsm_->IsClosed();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ж�peeer_conneciton�Ƿ��ڷǻ�Ծ״̬
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2014��1��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PeerConnection::IsInactive() const
{
	bool over_time = (alive_time_without_traffic_ >= max_alive_time_without_traffic_ ? true : false);
	
	if (over_time)
	{
		LOG(INFO) << "Peer Connection InActive With " << max_alive_time_without_traffic_ << " s";
	}
	
	return alive_time_without_traffic_ >= max_alive_time_without_traffic_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����have-piece��Ϣ
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��26��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
void PeerConnection::SendHaveMsg(uint64 piece_index)
{
	LOG(INFO) << "Send BT have msg | piece: " << piece_index;

	MemBufferPool& mem_pool = session_.mem_buf_pool();
	SendBuffer have_piece_buf = mem_pool.AllocMsgBuffer(MSG_BUF_COMMON);

	WriteHaveMsg(piece_index, have_piece_buf.buf, have_piece_buf.len);

	socket_connection()->SendData(have_piece_buf);
}

}
