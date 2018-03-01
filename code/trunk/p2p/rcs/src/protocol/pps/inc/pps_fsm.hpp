/*#############################################################################
 * �ļ���   : pps_fsm.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��30��
 * �ļ����� : PPSЭ��״̬�����������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PPS_FSM
#define HEADER_PPS_FSM

#include "p2p_fsm.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: PpsЭ��״̬��Initialize״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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
 * ����: PPSЭ��״̬��HandShake״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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
 * ����: PPSЭ��״̬��Metadata״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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
 * ����: PPSЭ��״̬��Block״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/01
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
 * ����: PPSЭ��״̬��Transfer״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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
 * ����: PPSЭ��״̬��Upload״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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
 * ����: PpsЭ��״̬��Download״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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
 * ����: PPSЭ��״̬��Seed״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/01
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
 * ����: PPSЭ��״̬��Close״̬
 * ���ߣ�tom_liu
 * ʱ�䣺2013/12/30
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

