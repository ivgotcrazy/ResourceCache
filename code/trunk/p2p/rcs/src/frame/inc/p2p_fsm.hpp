/*#############################################################################
 * 文件名   : p2p_fsm.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : 协议状态机相关声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 每一个PeerConnection都包含一个状态机，状态机是对P2P协议的一个抽象，定义了P2P
 * 协议的一个普遍框架和机制。事实上，P2P协议的各种实现可能不尽相同，这种可变性
 * 都放到了各个状态提供的虚拟函数去实现。状态机关注协议的状态变迁，所有关于协议
 * 状态切换的逻辑应该放在状态机中去实现；PeerConnection关注具体协议的细节，所有
 * 关于协议细节处理都应该放在PeerConnection中。
******************************************************************************/

namespace BroadCache
{

class FsmState;

typedef boost::shared_ptr<FsmState> FsmStateSP;
typedef std::vector<FsmStateSP> FsmStateVec;

/******************************************************************************
------------------------------- 状态机枚举定义 --------------------------------
******************************************************************************/

enum FsmPhaseType
{
	FSM_PHASE_ENTER,		// 进入状态机
	FSM_PHASE_IN,			// 开始运行
	FSM_PHASE_EXIT			// 跳出状态机
};

enum FsmMsgType
{
	FSM_MSG_NONE			= 0,  // 空消息
	FSM_MSG_TICK			= 1,  // 定时器
	FSM_MSG_PKT				= 2,  // 报文
	FSM_MSG_HANDSHAKED		= 3,  // 握手完成
	FSM_MSG_GET_MEATADATA	= 4,  // 获取元数据完成
	FSM_MSG_CHOKED			= 5,  // 本端被阻塞
	FSM_MSG_UNCHOKED		= 6,  // 本端被解阻塞
	FSM_MSG_INTEREST		= 7,  // 对端感兴趣消息
	FSM_MSG_NONINTEREST		= 8,  // 对端不感兴趣消息
	FSM_MSG_SEEDING			= 9,  // 开始做种
	FSM_MSG_CLOSE			= 10  // 关闭连接
};

enum FsmStateType
{
	FSM_STATE_INIT			= 0,  // 初始化状态
	FSM_STATE_HANDSHAKE		= 1,  // 握手状态
	FSM_STATE_METADATA		= 2,  // 元数据获取状态
	FSM_STATE_BLOCK			= 3,  // 阻塞状态
	FSM_STATE_TRANSFER		= 4,  // 双向传输状态
	FSM_STATE_UPLOAD		= 5,  // 上传状态
	FSM_STATE_DOWNLOAD		= 6,  // 下载状态
	FSM_STATE_SEED			= 7,  // 做种状态
	FSM_STATE_CLOSE			= 8,  // 关闭状态

	FSM_STATE_BUTT			// 非法状态
};

/******************************************************************************
 * 描述: 状态机声明
 * 作者：teck_zhou
 * 时间：2013/09/15
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

	// 协议模块需要初始化状态机的状态对象
	bool SetFsmStateObj(FsmStateType type, const FsmStateSP& state);

	// 消息驱动，状态机消息处理入口
	void ProcessMsg(FsmMsgType type, void* msg);
	
	bool IsClosed() const { return current_state_ == FSM_STATE_CLOSE; }
	FsmStateType current_state() const { return current_state_; }
	PeerConnection* peer_connection() const { return conn_; }
	EndPoint GetPeerEndPoint() const { return conn_->connection_id().remote; }	

private: // 内部状态调用接口
	bool JumpTo(FsmStateType state);
	bool IsValidJump(FsmStateType from, FsmStateType to);

private:
	FsmStateVec	states_; // 状态机状态对象
	PeerConnection* conn_; // 状态机所属连接
	TrafficController* traffic_controller_;
	FsmStateType current_state_; // 状态机当前所处状态
	bool is_vlaid_; // 状态机有效标志

	bool recv_unchoke_before_block_state_;
	bool recv_interest_before_block_state_;

	boost::recursive_mutex fsm_mutex_;
};

/******************************************************************************
------------------------------- 状态机消息声明 --------------------------------
******************************************************************************/
struct FsmHavePieceMsg
{
	FsmHavePieceMsg(uint64 piece) : piece_index(piece) {}

	uint64 piece_index;
};

/******************************************************************************
------------------------------- 状态机状态声明 --------------------------------
******************************************************************************/

/******************************************************************************
 * 描述: 状态基类
 * 作者：teck_zhou
 * 时间：2013/09/15
 *****************************************************************************/
class FsmState
{
public:
	FsmState(P2PFsm *fsm) : fsm_(fsm) {}

	PeerConnection* peer_connection() const { return fsm_->peer_connection(); }
	virtual FsmStateType GetStateType() { return FSM_STATE_BUTT; }

	// 各具体状态实现
	virtual bool EnterState() = 0;
	virtual bool ProcessState(FsmMsgType type, void* msg) = 0;
	virtual bool ExitState() = 0;

protected:
	// 状态错误的归一处理
	void StateProcFail(FsmStateType state, FsmPhaseType phase);

protected:
	// 各具体协议模块实现
	virtual bool EnterProc() = 0;
	virtual bool InProc(FsmMsgType type, void* msg) = 0;
	virtual bool ExitProc() = 0;

protected:
	P2PFsm*	fsm_;
};

/******************************************************************************
 * 描述: Initialize状态
 * 作者：teck_zhou
 * 时间：2013/09/15
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
 * 描述: HandShake状态
 * 作者：teck_zhou
 * 时间：2013/09/15
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
	bool sent_handshake_;		// 已经发送了握手报文
	bool received_handshake_;	// 已经收到了握手报文
};

/******************************************************************************
 * 描述: Metadata状态
 * 作者：teck_zhou
 * 时间：2013/09/15
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
 * 描述: Block状态
 * 作者：teck_zhou
 * 时间：2013/11/1
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
 * 描述: Transfer状态
 * 作者：teck_zhou
 * 时间：2013/09/15
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
 * 描述: Upload状态
 * 作者：teck_zhou
 * 时间：2013/09/15
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
 * 描述: Download状态
 * 作者：teck_zhou
 * 时间：2013/09/15
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
 * 描述: Seed状态
 * 作者：teck_zhou
 * 时间：2013/11/1
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
 * 描述: Close状态
 * 作者：teck_zhou
 * 时间：2013/09/15
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
