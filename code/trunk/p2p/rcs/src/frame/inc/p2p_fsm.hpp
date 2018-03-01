/*#############################################################################
 * �ļ���   : p2p_fsm.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : Э��״̬���������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_P2P_FSM
#define HEADER_P2P_FSM

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "peer.hpp"
#include "peer_connection.hpp"
#include "socket_connection.hpp"

/******************************************************************************
 * ÿһ��PeerConnection������һ��״̬����״̬���Ƕ�P2PЭ���һ�����󣬶�����P2P
 * Э���һ���ձ��ܺͻ��ơ���ʵ�ϣ�P2PЭ��ĸ���ʵ�ֿ��ܲ�����ͬ�����ֿɱ���
 * ���ŵ��˸���״̬�ṩ�����⺯��ȥʵ�֡�״̬����עЭ���״̬��Ǩ�����й���Э��
 * ״̬�л����߼�Ӧ�÷���״̬����ȥʵ�֣�PeerConnection��ע����Э���ϸ�ڣ�����
 * ����Э��ϸ�ڴ���Ӧ�÷���PeerConnection�С�
******************************************************************************/

namespace BroadCache
{

class FsmState;

typedef boost::shared_ptr<FsmState> FsmStateSP;
typedef std::vector<FsmStateSP> FsmStateVec;

/******************************************************************************
------------------------------- ״̬��ö�ٶ��� --------------------------------
******************************************************************************/

enum FsmPhaseType
{
	FSM_PHASE_ENTER,		// ����״̬��
	FSM_PHASE_IN,			// ��ʼ����
	FSM_PHASE_EXIT			// ����״̬��
};

enum FsmMsgType
{
	FSM_MSG_NONE			= 0,  // ����Ϣ
	FSM_MSG_TICK			= 1,  // ��ʱ��
	FSM_MSG_PKT				= 2,  // ����
	FSM_MSG_HANDSHAKED		= 3,  // �������
	FSM_MSG_GET_MEATADATA	= 4,  // ��ȡԪ�������
	FSM_MSG_CHOKED			= 5,  // ���˱�����
	FSM_MSG_UNCHOKED		= 6,  // ���˱�������
	FSM_MSG_INTEREST		= 7,  // �Զ˸���Ȥ��Ϣ
	FSM_MSG_NONINTEREST		= 8,  // �Զ˲�����Ȥ��Ϣ
	FSM_MSG_SEEDING			= 9,  // ��ʼ����
	FSM_MSG_CLOSE			= 10  // �ر�����
};

enum FsmStateType
{
	FSM_STATE_INIT			= 0,  // ��ʼ��״̬
	FSM_STATE_HANDSHAKE		= 1,  // ����״̬
	FSM_STATE_METADATA		= 2,  // Ԫ���ݻ�ȡ״̬
	FSM_STATE_BLOCK			= 3,  // ����״̬
	FSM_STATE_TRANSFER		= 4,  // ˫����״̬
	FSM_STATE_UPLOAD		= 5,  // �ϴ�״̬
	FSM_STATE_DOWNLOAD		= 6,  // ����״̬
	FSM_STATE_SEED			= 7,  // ����״̬
	FSM_STATE_CLOSE			= 8,  // �ر�״̬

	FSM_STATE_BUTT			// �Ƿ�״̬
};

/******************************************************************************
 * ����: ״̬������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class P2PFsm
{
public:
	friend class FsmInitState;
	friend class FsmHandshakeState;
	friend class FsmMetadataState;
	friend class FsmBlockState;
	friend class FsmTransferState;
	friend class FsmUploadState;
	friend class FsmDownloadState;
	friend class FsmSeedState;
	friend class FsmCloseState;

	P2PFsm(PeerConnection* conn, TrafficController* traffic_controller);
	~P2PFsm();

	bool StartFsm();
	bool StopFsm();

	// Э��ģ����Ҫ��ʼ��״̬����״̬����
	bool SetFsmStateObj(FsmStateType type, const FsmStateSP& state);

	// ��Ϣ������״̬����Ϣ�������
	void ProcessMsg(FsmMsgType type, void* msg);
	
	bool IsClosed() const { return current_state_ == FSM_STATE_CLOSE; }
	FsmStateType current_state() const { return current_state_; }
	PeerConnection* peer_connection() const { return conn_; }
	EndPoint GetPeerEndPoint() const { return conn_->connection_id().remote; }	

private: // �ڲ�״̬���ýӿ�
	bool JumpTo(FsmStateType state);
	bool IsValidJump(FsmStateType from, FsmStateType to);

private:
	FsmStateVec	states_; // ״̬��״̬����
	PeerConnection* conn_; // ״̬����������
	TrafficController* traffic_controller_;
	FsmStateType current_state_; // ״̬����ǰ����״̬
	bool is_vlaid_; // ״̬����Ч��־

	bool recv_unchoke_before_block_state_;
	bool recv_interest_before_block_state_;

	boost::recursive_mutex fsm_mutex_;
};

/******************************************************************************
------------------------------- ״̬����Ϣ���� --------------------------------
******************************************************************************/
struct FsmHavePieceMsg
{
	FsmHavePieceMsg(uint64 piece) : piece_index(piece) {}

	uint64 piece_index;
};

/******************************************************************************
------------------------------- ״̬��״̬���� --------------------------------
******************************************************************************/

/******************************************************************************
 * ����: ״̬����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmState
{
public:
	FsmState(P2PFsm *fsm) : fsm_(fsm) {}

	PeerConnection* peer_connection() const { return fsm_->peer_connection(); }
	virtual FsmStateType GetStateType() { return FSM_STATE_BUTT; }

	// ������״̬ʵ��
	virtual bool EnterState() = 0;
	virtual bool ProcessState(FsmMsgType type, void* msg) = 0;
	virtual bool ExitState() = 0;

protected:
	// ״̬����Ĺ�һ����
	void StateProcFail(FsmStateType state, FsmPhaseType phase);

protected:
	// ������Э��ģ��ʵ��
	virtual bool EnterProc() = 0;
	virtual bool InProc(FsmMsgType type, void* msg) = 0;
	virtual bool ExitProc() = 0;

protected:
	P2PFsm*	fsm_;
};

/******************************************************************************
 * ����: Initialize״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmInitState : public FsmState
{
public:
	friend class P2PFsm;
	FsmInitState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_INIT; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();
};

/******************************************************************************
 * ����: HandShake״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmHandshakeState : public FsmState
{
public:
	friend class P2PFsm;
	FsmHandshakeState(P2PFsm *fsm) : FsmState(fsm), sent_handshake_(false), 
        received_handshake_(false) {}
	
	FsmStateType GetStateType() { return FSM_STATE_HANDSHAKE; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();

protected:
	bool sent_handshake_;		// �Ѿ����������ֱ���
	bool received_handshake_;	// �Ѿ��յ������ֱ���
};

/******************************************************************************
 * ����: Metadata״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmMetadataState : public FsmState
{
public:
	friend class P2PFsm;
 	FsmMetadataState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_METADATA; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();
};

/******************************************************************************
 * ����: Block״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/1
 *****************************************************************************/
class FsmBlockState : public FsmState
{
public:
	friend class P2PFsm;
 	FsmBlockState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_BLOCK; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();

};

/******************************************************************************
 * ����: Transfer״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmTransferState : public FsmState
{
public:
	friend class P2PFsm;
 	FsmTransferState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_TRANSFER; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();

};

/******************************************************************************
 * ����: Upload״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmUploadState : public FsmState
{
public:
	friend class P2PFsm;
 	FsmUploadState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_UPLOAD; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();
};

/******************************************************************************
 * ����: Download״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmDownloadState : public FsmState
{
public:
	friend class P2PFsm;
 	FsmDownloadState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_DOWNLOAD; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();
};

/******************************************************************************
 * ����: Seed״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/1
 *****************************************************************************/
class FsmSeedState : public FsmState
{
public:
	friend class P2PFsm;
 	FsmSeedState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_SEED; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();

};

/******************************************************************************
 * ����: Close״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
class FsmCloseState : public FsmState
{
public:
	friend class P2PFsm;
	FsmCloseState(P2PFsm *fsm) : FsmState(fsm) {}

	FsmStateType GetStateType() { return FSM_STATE_CLOSE; }

	bool EnterState();
	bool ProcessState(FsmMsgType type, void* msg);
	bool ExitState();

protected:
	bool is_closing_;
};

}

#endif
