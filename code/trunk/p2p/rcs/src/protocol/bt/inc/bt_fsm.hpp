/*#############################################################################
 * 文件名   : bt_fsm.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月16日
 * 文件描述 : BT协议状态机相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BT_FSM
#define HEADER_BT_FSM

#include "p2p_fsm.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: BT协议状态机Initialize状态
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtFsmInitState : public FsmInitState
{
public:
	BtFsmInitState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmInitState(fsm), bt_conn_(conn) {}
	
private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机HandShake状态
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtFsmHandshakeState : public FsmHandshakeState
{
public:
	BtFsmHandshakeState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmHandshakeState(fsm), bt_conn_(conn) {}
	
private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机Metadata状态
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtFsmMetadataState : public FsmMetadataState
{
public:
	BtFsmMetadataState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmMetadataState(fsm), bt_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机Block状态
 * 作者：teck_zhou
 * 时间：2013/11/01
 *****************************************************************************/
class BtFsmBlockState : public FsmBlockState
{
public:
	BtFsmBlockState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmBlockState(fsm), bt_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机Transfer状态
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtFsmTransferState : public FsmTransferState
{
public:
	BtFsmTransferState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmTransferState(fsm), bt_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机Upload状态
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtFsmUploadState : public FsmUploadState
{
public:
	BtFsmUploadState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmUploadState(fsm), bt_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机Download状态
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtFsmDownloadState : public FsmDownloadState
{
public:
	BtFsmDownloadState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmDownloadState(fsm), bt_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机Seed状态
 * 作者：teck_zhou
 * 时间：2013/11/01
 *****************************************************************************/
class BtFsmSeedState : public FsmSeedState
{
public:
	BtFsmSeedState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmSeedState(fsm), bt_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};

/******************************************************************************
 * 描述: BT协议状态机Close状态
 * 作者：teck_zhou
 * 时间：2013/09/16
 *****************************************************************************/
class BtFsmCloseState : public FsmCloseState
{
public:
	BtFsmCloseState(P2PFsm *fsm, BtPeerConnection* conn) 
		: FsmCloseState(fsm), bt_conn_(conn) {}

private:
	virtual bool EnterProc();
	virtual bool InProc(FsmMsgType type, void* msg);
	virtual bool ExitProc();

private:
	BtPeerConnection* bt_conn_;
};


}

#endif
