/*#############################################################################
 * 文件名   : pps_fsm.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年12月30日
 * 文件描述 : PPS协议状态机相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PPS_FSM
#define HEADER_PPS_FSM

#include "p2p_fsm.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: Pps协议状态机Initialize状态
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsFsmInitState : public FsmInitState
{
public:
	PpsFsmInitState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmInitState(fsm), pps_conn_(conn) {}
	
private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: PPS协议状态机HandShake状态
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsFsmHandshakeState : public FsmHandshakeState
{
public:
	PpsFsmHandshakeState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmHandshakeState(fsm), pps_conn_(conn) {}
	
private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: PPS协议状态机Metadata状态
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsFsmMetadataState : public FsmMetadataState
{
public:
	PpsFsmMetadataState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmMetadataState(fsm), pps_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: PPS协议状态机Block状态
 * 作者：tom_liu
 * 时间：2013/11/01
 *****************************************************************************/
class PpsFsmBlockState : public FsmBlockState
{
public:
	PpsFsmBlockState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmBlockState(fsm), pps_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: PPS协议状态机Transfer状态
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsFsmTransferState : public FsmTransferState
{
public:
	PpsFsmTransferState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmTransferState(fsm), pps_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: PPS协议状态机Upload状态
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsFsmUploadState : public FsmUploadState
{
public:
	PpsFsmUploadState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmUploadState(fsm), pps_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: Pps协议状态机Download状态
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsFsmDownloadState : public FsmDownloadState
{
public:
	PpsFsmDownloadState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmDownloadState(fsm), pps_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: PPS协议状态机Seed状态
 * 作者：tom_liu
 * 时间：2013/11/01
 *****************************************************************************/
class PpsFsmSeedState : public FsmSeedState
{
public:
	PpsFsmSeedState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmSeedState(fsm), pps_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};

/******************************************************************************
 * 描述: PPS协议状态机Close状态
 * 作者：tom_liu
 * 时间：2013/12/30
 *****************************************************************************/
class PpsFsmCloseState : public FsmCloseState
{
public:
	PpsFsmCloseState(P2PFsm *fsm, PpsPeerConnection* conn) 
		: FsmCloseState(fsm), pps_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	PpsPeerConnection* pps_conn_;
};


}

#endif

