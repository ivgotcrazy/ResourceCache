/*#############################################################################
 * 文件名   : p2p_fsm.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : 协议状态机相关实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描  述: 构造函数
 * 参  数: [in] conn 连接
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
P2PFsm::P2PFsm(PeerConnection* conn, TrafficController* traffic_controller) 
	: states_(FSM_STATE_BUTT)
	, conn_(conn)
	, traffic_controller_(traffic_controller)
	, current_state_(FSM_STATE_INIT)
{

}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
P2PFsm::~P2PFsm()
{
	LOG(INFO) << ">>> Destructing P2P FSM";

	// 如果一个线程已经申请了mutex，另一个线程尝试去释放会有问题
	// 因此这里在销毁P2PFsm时，需要先获取锁，保证锁不被其他线程占用
	boost::recursive_mutex::scoped_lock lock(fsm_mutex_);

	StopFsm();
}

/*-----------------------------------------------------------------------------
 * 描  述: 消息处理入口
 * 参  数: [in] type 消息类型
 *         [in] msg 消息数据
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void P2PFsm::ProcessMsg(FsmMsgType msg_type, void* msg)
{
	boost::recursive_mutex::scoped_lock lock(fsm_mutex_);

	if (msg_type == FSM_MSG_CLOSE)
	{
		JumpTo(FSM_STATE_CLOSE);
		return;
	}

	// unchoke和interest消息有可能在block状态前收到，需要记录下
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
 * 描  述: 状态机状态跃迁
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool P2PFsm::JumpTo(FsmStateType state)
{
	if (!IsValidJump(current_state_, state))
	{
		LOG(ERROR) << "Invalid jump from " << current_state_ << " to " << state;
		return false;
	}

	// 执行当前状态的Exit处理
	if (!(states_[current_state_]->ExitState()))
	{
		LOG(ERROR) << "Fail to exit state: " << current_state_;
		return false;
	}

	// 修改当前状态
	current_state_ = state;

	// 下一跳转状态的Enter处理
	if (!(states_[current_state_]->EnterState()))
	{
		LOG(ERROR) << "Fail to enter state: " << current_state_;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 是否是合法的状态跃迁
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool P2PFsm::IsValidJump(FsmStateType from, FsmStateType to)
{
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动状态机
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool P2PFsm::StartFsm()
{
	// 确认状态对象是否设置好
	if (states_.size() != FSM_STATE_BUTT)
	{
		LOG(ERROR) << "Incorrect state object number: " << states_.size();
		return false;
	}

	boost::recursive_mutex::scoped_lock lock(fsm_mutex_);

	BC_ASSERT(current_state_ == FSM_STATE_INIT);

	// 进入INIT状态
	if (!(states_[current_state_]->EnterState()))
	{
		LOG(ERROR) << "Fail to enter FSM_STATE_INIT";
		return false;
	}

	// 模拟消息处理INIT状态
	if (!(states_[current_state_]->ProcessState(FSM_MSG_NONE, 0)))
	{
		LOG(ERROR) << "Fail to process FSM_STATE_INIT";
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 停止状态机
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 设置状态对象
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
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
 * 描  述: 状态机状态处理失败的统一处理函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void FsmState::StateProcFail(FsmStateType state_type, FsmPhaseType phase)
{
}

//===============================FsmInitState==================================

/*-----------------------------------------------------------------------------
 * 描  述: 初始状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmInitState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_INIT");

	// do something initializing

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_INIT, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始状态处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmInitState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_INIT", msg_type);

	// do something

	// 调用协议模块实现
	CALL_IN_PROC(FSM_STATE_INIT, FSM_PHASE_IN, msg_type, msg);

	// 无条件转移到FSM_STATE_HANDSHAKING状态
	JUMP_TO_STATE(FSM_STATE_HANDSHAKE);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始状态后处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmInitState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_INIT");

	// do something

	// 调用协议模块实现
	CALL_EXIT_PROC(FSM_STATE_INIT, FSM_PHASE_EXIT);

	return true;
}

//============================FsmHandshakeState================================

/*-----------------------------------------------------------------------------
 * 描  述: 握手状态前处理，协议模块应该发送握手消息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmHandshakeState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_HANDSHAKE");

	// do something

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_HANDSHAKE, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 握手状态处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmHandshakeState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_HANDSHAKE", msg_type);

	// 握手完成后转移到Metadata状态
	if (msg_type == FSM_MSG_HANDSHAKED)
	{
		JUMP_TO_STATE(FSM_STATE_METADATA);
		return true;
	}

	// 调用协议模块实现
	CALL_IN_PROC(FSM_STATE_HANDSHAKE, FSM_PHASE_IN, msg_type, msg);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 握手状态后处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmHandshakeState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_HANDSHAKE");

	// 先调用协议模块实现
	CALL_EXIT_PROC(FSM_STATE_HANDSHAKE, FSM_PHASE_EXIT);

	// do something

	return true;
}

//=============================FsmMetadataState================================

/*-----------------------------------------------------------------------------
 * 描  述: 元数据状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmMetadataState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_METADATA");

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_METADATA, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 元数据状态处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmMetadataState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_METADATA", msg_type);

	// 获取Metadata后转移到Transfer状态
	if (msg_type == FSM_MSG_GET_MEATADATA)
	{
		JUMP_TO_STATE(FSM_STATE_BLOCK);
		return true;
	}

	if (msg_type != FSM_MSG_PKT && msg_type != FSM_MSG_TICK)
	{
		return true;
	}

	// 调用协议模块实现
	CALL_IN_PROC(FSM_STATE_METADATA, FSM_PHASE_IN, msg_type, msg);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 元数据状态后处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmMetadataState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_METADATA");

	// 调用协议模块实现
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
 * 描  述: 阻塞状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmBlockState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_BLOCK");

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_BLOCK, FSM_PHASE_ENTER);

	TorrentSP torrent = fsm_->peer_connection()->torrent().lock();
	if (!torrent) return false;

	// 如果已经做种，则直接跳到seed状态
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
 * 描  述: 阻塞状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmBlockState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_BLOCK", msg_type);

	// 消息处理
	switch (msg_type)
	{
	case FSM_MSG_UNCHOKED:
		JUMP_TO_STATE(FSM_STATE_DOWNLOAD);
		break;

	case FSM_MSG_INTEREST:
		JUMP_TO_STATE(FSM_STATE_UPLOAD);
		break;

	case FSM_MSG_PKT: // 调用协议模块实现
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
 * 描  述: 阻塞状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmBlockState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_BLOCK");

	// 调用协议模块实现
	CALL_EXIT_PROC(FSM_STATE_BLOCK, FSM_PHASE_EXIT);

	// do something

	return true;
}

//=============================FsmTransferState================================

/*-----------------------------------------------------------------------------
 * 描  述: 传输状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmTransferState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_TRANSFER");

	// do something initializing

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_TRANSFER, FSM_PHASE_ENTER);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 传输状态处理
 * 参  数: [in] msg_type 消息类型
 *         [in] msg 消息数据
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmTransferState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_TRANSFER", msg_type);

	// 消息处理
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
 * 描  述: 传输状态退出处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmTransferState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_TRANSFER");

	// 先调用协议模块实现
	CALL_EXIT_PROC(FSM_STATE_TRANSFER, FSM_PHASE_EXIT);

	// do something

	return true;
}

//===============================FsmUploadState================================

/*-----------------------------------------------------------------------------
 * 描  述: 空闲状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmUploadState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_UPLOAD");

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_UPLOAD, FSM_PHASE_ENTER);

	// 停止上传
	fsm_->traffic_controller_->StopDownload();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 空闲状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmUploadState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_UPLOAD", msg_type);

	// 消息处理
	switch (msg_type)
	{
	case FSM_MSG_TICK:
		fsm_->traffic_controller_->TryProcPeerRequest();
		break;

	case FSM_MSG_PKT: // 调用协议模块实现
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
 * 描  述: 空闲状态处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmUploadState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_UPLOAD");

	// do something

	// 调用协议模块实现
	CALL_EXIT_PROC(FSM_STATE_UPLOAD, FSM_PHASE_EXIT);

	return true;
}

//==============================FsmDownloadState===============================

/*-----------------------------------------------------------------------------
 * 描  述: 空闲状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmDownloadState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_DOWNLOAD");

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_DOWNLOAD, FSM_PHASE_ENTER);

	// 先停止下载
	fsm_->traffic_controller_->StopUpload();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 空闲状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmDownloadState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_DOWNLOAD", msg_type);

	// 消息处理
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

	case FSM_MSG_PKT: // 调用协议模块实现
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
 * 描  述: 空闲状态处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmDownloadState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_DOWNLOAD");

	// do something

	// 调用协议模块实现
	CALL_EXIT_PROC(FSM_STATE_DOWNLOAD, FSM_PHASE_EXIT);

	return true;
}

//===============================FsmSeedState==================================

/*-----------------------------------------------------------------------------
 * 描  述: 做种状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmSeedState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_SEED");

	// 调用协议模块实现
	CALL_ENTER_PROC(FSM_STATE_SEED, FSM_PHASE_ENTER);

	fsm_->traffic_controller_->StopDownload();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 做种状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmSeedState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_SEED", msg_type);

	// 消息处理
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
 * 描  述: 做种状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmSeedState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_SEED");

	// do something

	// 调用协议模块实现
	CALL_EXIT_PROC(FSM_STATE_SEED, FSM_PHASE_EXIT);

	return true;
}

//===============================FsmCloseState=================================

/*-----------------------------------------------------------------------------
 * 描  述: 关闭状态前处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmCloseState::EnterState()
{
	LOG_FSM_ENTER("FSM_STATE_CLOSE");

	// do something initializing

	// 调用协议模块实现
	if (!EnterProc()) LOG(ERROR) << "Fail to enter  FSM_STATE_CLOSE"; 

	// 触发close状态处理
	(void)ProcessState(FSM_MSG_NONE, 0);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 关闭状态处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmCloseState::ProcessState(FsmMsgType msg_type, void* msg)
{
	LOG_FSM_PROC("FSM_STATE_CLOSE", msg_type);

	// close阶段不处理任何消息
	if (FSM_MSG_NONE != msg_type)
	{
		LOG(INFO) << "Received message " << msg_type << " in FSM_STATE_CLOSE"; 
		return true;
	}

	// 停止上传和下载
	fsm_->traffic_controller_->StopDownload();
	fsm_->traffic_controller_->StopUpload();

	// 调用协议模块实现
	if (!InProc(msg_type, msg))
	{
		LOG(ERROR) << "Fail to process state | MsgType: " << msg_type;
	}

	// 直接进行后处理
	ExitState();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 关闭状态后处理
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年09月05日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool FsmCloseState::ExitState()
{
	LOG_FSM_EXIT("FSM_STATE_CLOSE");

	// do something

	if (!ExitProc()) LOG(ERROR) << "Fail to exit FSM_STATE_CLOSE";

	return true;
}

}
