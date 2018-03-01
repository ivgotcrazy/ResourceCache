/*#############################################################################
 * �ļ���   : p2p_fsm.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : Э��״̬�����ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "p2p_fsm.hpp"
#include "bc_typedef.hpp"
#include "peer_connection.hpp"
#include "bc_assert.hpp"
#include "endpoint.hpp"
#include "torrent.hpp"


#define CALL_ENTER_PROC(FsmState, FsmStatePhase) \
	if (!EnterProc()) \
	{ \
		LOG(ERROR) << "Fail to enter state " << FsmState; \
		StateProcFail(FsmState, FsmStatePhase); \
		return false; \
	}

#define CALL_IN_PROC(FsmState, FsmStatePhase, MsgType, Msg) \
	if (!InProc(MsgType, Msg)) \
	{ \
		LOG(ERROR) << "Fail to process state " << FsmState << " | MsgType: " << MsgType; \
		StateProcFail(FsmState, FsmStatePhase); \
		return false; \
	}

#define CALL_EXIT_PROC(FsmState, FsmStatePhase) \
	if (!ExitProc()) \
	{ \
		LOG(ERROR) << "Fail to exit state " << FsmState; \
		StateProcFail(FsmState, FsmStatePhase); \
		return false; \
	}

#define JUMP_TO_STATE(State) \
	if (!(fsm_->JumpTo(State))) \
	{ \
		LOG(ERROR) << "Fail to jump to " << State; \
		return false; \
	}

#define LOG_FSM_ENTER(state) \
	LOG(INFO) << "Enter " << state << " | " << fsm_->peer_connection()->connection_id();

#define LOG_FSM_PROC(state, msg_type) \
	if (msg_type != FSM_MSG_TICK) \
	{ \
		LOG(INFO) << "Process " << state << "(" << msg_type << ") | " \
				<< fsm_->peer_connection()->connection_id(); \
	}

#define LOG_FSM_EXIT(state) \
	LOG(INFO) << "Exit " << state << " | " << fsm_->peer_connection()->connection_id();

namespace BroadCache
{

//==================================P2PFsm=====================================

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: [in] conn ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
P2PFsm::P2PFsm(PeerConnection* conn, TrafficController* traffic_controller) 
	: states_(FSM_STATE_BUTT)
	, conn_(conn)
	, traffic_controller_(traffic_controller)
	, current_state_(FSM_STATE_INIT)
{

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
P2PFsm::~P2PFsm()
{
	LOG(INFO) << ">>> Destructing P2P FSM";

	// ���һ���߳��Ѿ�������mutex����һ���̳߳���ȥ�ͷŻ�������
	// �������������P2PFsmʱ����Ҫ�Ȼ�ȡ������֤�����������߳�ռ��
	boost::recursive_mutex::scoped_lock lock(fsm_mutex_);

	StopFsm();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��Ϣ�������
 * ��  ��: [in] type ��Ϣ����
 *         [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void P2PFsm::ProcessMsg(FsmMsgType msg_type, void* msg)
{
	boost::recursive_mutex::scoped_lock lock(fsm_mutex_);

	if (msg_type == FSM_MSG_CLOSE)
	{
		JumpTo(FSM_STATE_CLOSE);
		return;
	}

	// unchoke��interest��Ϣ�п�����block״̬ǰ�յ�����Ҫ��¼��
	if (msg_type == FSM_MSG_UNCHOKED && current_state_ < FSM_STATE_BLOCK)
	{
		recv_unchoke_before_block_state_ = true;
	}
	else if (msg_type == FSM_MSG_INTEREST && current_state_ < FSM_STATE_BLOCK)
	{
		recv_interest_before_block_state_ = true;
	}

	if (!(states_[current_state_]->ProcessState(msg_type, msg)))
	{
		LOG(ERROR) << "Fail to process message | msg_type: " << msg_type;
		return;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ״̬��״̬ԾǨ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool P2PFsm::JumpTo(FsmStateType state)
{
	if (!IsValidJump(current_state_, state))
	{
		LOG(ERROR) << "Invalid jump from " << current_state_ << " to " << state;
		return false;
	}

	// ִ�е�ǰ״̬��Exit����
	if (!(states_[current_state_]->ExitState()))
	{
		LOG(ERROR) << "Fail to exit state: " << current_state_;
		return false;
	}

	// �޸ĵ�ǰ״̬
	current_state_ = state;

	// ��һ��ת״̬��Enter����
	if (!(states_[current_state_]->EnterState()))
	{
		LOG(ERROR) << "Fail to enter state: " << current_state_;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƿ��ǺϷ���״̬ԾǨ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool P2PFsm::IsValidJump(FsmStateType from, FsmStateType to)
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool P2PFsm::StartFsm()
{
	// ȷ��״̬�����Ƿ����ú�
	if (states_.size() != FSM_STATE_BUTT)
	{
		LOG(ERROR) << "Incorrect state object number: " << states_.size();
		return false;
	}

	boost::recursive_mutex::scoped_lock lock(fsm_mutex_);

	BC_ASSERT(current_state_ == FSM_STATE_INIT);

	// ����INIT״̬
	if (!(states_[current_state_]->EnterState()))
	{
		LOG(ERROR) << "Fail to enter FSM_STATE_INIT";
		return false;
	}

	// ģ����Ϣ����INIT״̬
	if (!(states_[current_state_]->ProcessState(FSM_MSG_NONE, 0)))
	{
		LOG(ERROR) << "Fail to process FSM_STATE_INIT";
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣ״̬��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool P2PFsm::StopFsm()
{
	if (current_state_ == FSM_STATE_CLOSE) return true;

	if (!JumpTo(FSM_STATE_CLOSE))
	{
		LOG(ERROR) << "Fail to jump to FSM_STATE_CLOSED";
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool P2PFsm::SetFsmStateObj(FsmStateType state_type, const FsmStateSP& state)
{
	if ((state_type >= FSM_STATE_BUTT) || !state)
	{
		LOG(ERROR) << "Set FSM state error | type: " << state_type;
		return false;
	}

	states_[state_type] = state;

	return true;
}

//=================================FsmState====================================

/*-----------------------------------------------------------------------------
 * ��  ��: ״̬��״̬����ʧ�ܵ�ͳһ������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void FsmState::StateProcFail(FsmStateType state_type, FsmPhaseType phase)
{
}

//===============================FsmInitState==================================

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmInitState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_INIT");

	// do something initializing

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_INIT, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmInitState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_INIT", msg_type);

	// do something

	// ����Э��ģ��ʵ��
	CALL_IN_PROC(FSM_STATE_INIT, FSM_PHASE_IN, msg_type, msg);

	// ������ת�Ƶ�FSM_STATE_HANDSHAKING״̬
	JUMP_TO_STATE(FSM_STATE_HANDSHAKE);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmInitState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_INIT");

	// do something

	// ����Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_INIT, FSM_PHASE_EXIT);

	return true;
}

//============================FsmHandshakeState================================

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����Э��ģ��Ӧ�÷���������Ϣ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmHandshakeState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_HANDSHAKE");

	// do something

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_HANDSHAKE, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmHandshakeState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_HANDSHAKE", msg_type);

	// ������ɺ�ת�Ƶ�Metadata״̬
	if (msg_type == FSM_MSG_HANDSHAKED)
	{
		JUMP_TO_STATE(FSM_STATE_METADATA);
		return true;
	}

	// ����Э��ģ��ʵ��
	CALL_IN_PROC(FSM_STATE_HANDSHAKE, FSM_PHASE_IN, msg_type, msg);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmHandshakeState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_HANDSHAKE");

	// �ȵ���Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_HANDSHAKE, FSM_PHASE_EXIT);

	// do something

	return true;
}

//=============================FsmMetadataState================================

/*-----------------------------------------------------------------------------
 * ��  ��: Ԫ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmMetadataState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_METADATA");

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_METADATA, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Ԫ����״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmMetadataState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_METADATA", msg_type);

	// ��ȡMetadata��ת�Ƶ�Transfer״̬
	if (msg_type == FSM_MSG_GET_MEATADATA)
	{
		JUMP_TO_STATE(FSM_STATE_BLOCK);
		return true;
	}

	if (msg_type != FSM_MSG_PKT && msg_type != FSM_MSG_TICK)
	{
		return true;
	}

	// ����Э��ģ��ʵ��
	CALL_IN_PROC(FSM_STATE_METADATA, FSM_PHASE_IN, msg_type, msg);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Ԫ����״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmMetadataState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_METADATA");

	// ����Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_METADATA, FSM_PHASE_EXIT);

	TorrentSP torrent = fsm_->peer_connection()->torrent().lock();
	if (!torrent) return true;

	if (torrent->is_ready())
	{
		fsm_->peer_connection()->InitBitfield(torrent->GetPieceNum());
	}

	return true;
}

//===============================FsmBlockState=================================

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmBlockState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_BLOCK");

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_BLOCK, FSM_PHASE_ENTER);

	TorrentSP torrent = fsm_->peer_connection()->torrent().lock();
	if (!torrent) return false;

	// ����Ѿ����֣���ֱ������seed״̬
	if (torrent->is_seed())
	{
		JUMP_TO_STATE(FSM_STATE_SEED);
		return true;
	}

	if (fsm_->recv_unchoke_before_block_state_ 
		&& fsm_->recv_interest_before_block_state_)
	{
		JUMP_TO_STATE(FSM_STATE_TRANSFER);
		return true;
	}
	else if (fsm_->recv_unchoke_before_block_state_)
	{
		JUMP_TO_STATE(FSM_STATE_DOWNLOAD);
		return true;
	}
	else if (fsm_->recv_interest_before_block_state_)
	{
		JUMP_TO_STATE(FSM_STATE_UPLOAD);
		return true;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmBlockState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_BLOCK", msg_type);

	// ��Ϣ����
	switch (msg_type)
	{
	case FSM_MSG_UNCHOKED:
		JUMP_TO_STATE(FSM_STATE_DOWNLOAD);
		break;

	case FSM_MSG_INTEREST:
		JUMP_TO_STATE(FSM_STATE_UPLOAD);
		break;

	case FSM_MSG_PKT: // ����Э��ģ��ʵ��
		CALL_IN_PROC(FSM_STATE_BLOCK, FSM_PHASE_IN, msg_type, msg);
		break;

	case FSM_MSG_TICK:
		break;

	default:
		LOG(WARNING) << "Received unexpected msg(" << msg_type << ")";
		break;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmBlockState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_BLOCK");

	// ����Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_BLOCK, FSM_PHASE_EXIT);

	// do something

	return true;
}

//=============================FsmTransferState================================

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmTransferState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_TRANSFER");

	// do something initializing

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_TRANSFER, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬����
 * ��  ��: [in] msg_type ��Ϣ����
 *         [in] msg ��Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmTransferState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_TRANSFER", msg_type);

	// ��Ϣ����
	switch (msg_type)
	{
	case FSM_MSG_TICK:
		fsm_->traffic_controller_->TryProcPeerRequest();
		fsm_->traffic_controller_->TrySendDataRequest();
		break;

	case FSM_MSG_CHOKED:
		JUMP_TO_STATE(FSM_STATE_UPLOAD);
		break;

	case FSM_MSG_NONINTEREST:
		JUMP_TO_STATE(FSM_STATE_DOWNLOAD);
		break;

	case FSM_MSG_PKT:
		CALL_IN_PROC(FSM_STATE_TRANSFER, FSM_PHASE_IN, msg_type, msg);
		break;

	case FSM_MSG_SEEDING:
		JUMP_TO_STATE(FSM_STATE_SEED);
		break;

	default:
		LOG(WARNING) << "Received unexpected msg(" << msg_type << ")";
		break;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬�˳�����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmTransferState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_TRANSFER");

	// �ȵ���Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_TRANSFER, FSM_PHASE_EXIT);

	// do something

	return true;
}

//===============================FsmUploadState================================

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmUploadState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_UPLOAD");

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_UPLOAD, FSM_PHASE_ENTER);

	// ֹͣ�ϴ�
	fsm_->traffic_controller_->StopDownload();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmUploadState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_UPLOAD", msg_type);

	// ��Ϣ����
	switch (msg_type)
	{
	case FSM_MSG_TICK:
		fsm_->traffic_controller_->TryProcPeerRequest();
		break;

	case FSM_MSG_PKT: // ����Э��ģ��ʵ��
		CALL_IN_PROC(FSM_STATE_UPLOAD, FSM_PHASE_IN, msg_type, msg);
		break;

	case FSM_MSG_NONINTEREST:
		JUMP_TO_STATE(FSM_STATE_CLOSE);
		break;

	case FSM_MSG_UNCHOKED:
		JUMP_TO_STATE(FSM_STATE_TRANSFER);
		break;
		
	default:
		LOG(WARNING) << "Received unexpected msg(" << msg_type << ")";
		break;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmUploadState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_UPLOAD");

	// do something

	// ����Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_UPLOAD, FSM_PHASE_EXIT);

	return true;
}

//==============================FsmDownloadState===============================

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmDownloadState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_DOWNLOAD");

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_DOWNLOAD, FSM_PHASE_ENTER);

	// ��ֹͣ����
	fsm_->traffic_controller_->StopUpload();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmDownloadState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_DOWNLOAD", msg_type);

	// ��Ϣ����
	switch (msg_type)
	{
	case FSM_MSG_TICK:
		fsm_->traffic_controller_->TrySendDataRequest();
		break;

	case FSM_MSG_CHOKED:
		JUMP_TO_STATE(FSM_STATE_CLOSE);
		break;

	case FSM_MSG_INTEREST:
		JUMP_TO_STATE(FSM_STATE_TRANSFER);
		break;

	case FSM_MSG_PKT: // ����Э��ģ��ʵ��
		CALL_IN_PROC(FSM_STATE_DOWNLOAD, FSM_PHASE_IN, msg_type, msg);
		break;

	case FSM_MSG_SEEDING:
		JUMP_TO_STATE(FSM_STATE_SEED);
		break;

	default:
		LOG(WARNING) << "Received unexpected msg(" << msg_type << ")";
		break;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmDownloadState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_DOWNLOAD");

	// do something

	// ����Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_DOWNLOAD, FSM_PHASE_EXIT);

	return true;
}

//===============================FsmSeedState==================================

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmSeedState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_SEED");

	// ����Э��ģ��ʵ��
	CALL_ENTER_PROC(FSM_STATE_SEED, FSM_PHASE_ENTER);

	fsm_->traffic_controller_->StopDownload();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmSeedState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_SEED", msg_type);

	// ��Ϣ����
	switch (msg_type)
	{
	case FSM_MSG_TICK:
		fsm_->traffic_controller_->TryProcPeerRequest();
		break;

	case FSM_MSG_PKT:
		CALL_IN_PROC(FSM_STATE_SEED, FSM_PHASE_IN, msg_type, msg);
		break;

	default:
		LOG(WARNING) << "Received unexpected msg(" << msg_type << ")";
		break;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmSeedState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_SEED");

	// do something

	// ����Э��ģ��ʵ��
	CALL_EXIT_PROC(FSM_STATE_SEED, FSM_PHASE_EXIT);

	return true;
}

//===============================FsmCloseState=================================

/*-----------------------------------------------------------------------------
 * ��  ��: �ر�״̬ǰ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmCloseState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_CLOSE");

	// do something initializing

	// ����Э��ģ��ʵ��
	if (!EnterProc()) LOG(ERROR) << "Fail to enter  FSM_STATE_CLOSE"; 

	// ����close״̬����
	(void)ProcessState(FSM_MSG_NONE, 0);

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ر�״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmCloseState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_CLOSE", msg_type);

	// close�׶β������κ���Ϣ
	if (FSM_MSG_NONE != msg_type)
	{
		LOG(INFO) << "Received message " << msg_type << " in FSM_STATE_CLOSE"; 
		return true;
	}

	// ֹͣ�ϴ�������
	fsm_->traffic_controller_->StopDownload();
	fsm_->traffic_controller_->StopUpload();

	// ����Э��ģ��ʵ��
	if (!InProc(msg_type, msg))
	{
		LOG(ERROR) << "Fail to process state | MsgType: " << msg_type;
	}

	// ֱ�ӽ��к���
	ExitState();

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ر�״̬����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��09��05��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool FsmCloseState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_CLOSE");

	// do something

	if (!ExitProc()) LOG(ERROR) << "Fail to exit FSM_STATE_CLOSE";

	return true;
}

}
