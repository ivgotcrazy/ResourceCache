/*#############################################################################
 * �ļ���   : bt_fsm.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��16��
 * �ļ����� : BTЭ��״̬�����������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BT_FSM
#define HEADER_BT_FSM

#include "p2p_fsm.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: BTЭ��״̬��Initialize״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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
 * ����: BTЭ��״̬��HandShake״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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
 * ����: BTЭ��״̬��Metadata״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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
 * ����: BTЭ��״̬��Block״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/01
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
 * ����: BTЭ��״̬��Transfer״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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
 * ����: BTЭ��״̬��Upload״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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
 * ����: BTЭ��״̬��Download״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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
 * ����: BTЭ��״̬��Seed״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/11/01
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
 * ����: BTЭ��״̬��Close״̬
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/16
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
