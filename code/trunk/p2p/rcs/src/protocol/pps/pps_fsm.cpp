/*------------------------------------------------------------------------------
 * �ļ���   : pps_fsm.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2014��1��2��
 * �ļ����� : PPSЭ��״̬�������ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/

#include "pps_fsm.hpp"
#include "pps_metadata_retriver.hpp"
#include "torrent.hpp"
#include "info_hash.hpp"
#include "peer_connection.hpp"
#include "pps_peer_connection.hpp"
#include "pps_torrent.hpp"
#include "mem_buffer_pool.hpp"
#include "socket_connection.hpp"
#include "session.hpp"
#include "rcs_config.hpp"
#include "bc_util.hpp"
#include "protocol_msg_macro.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

MSG_MAP_BEGIN(PpsFsmHandshakeState, 0)
MSG_MAP(PPS_MSG_HANDSHAKE, &PpsPeerConnection::OnHandshakeMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(PpsFsmMetadataState, 0)
MSG_MAP(PPS_MSG_BITFIELD, &PpsPeerConnection::OnBitfieldMsg)
MSG_MAP(PPS_MSG_REQUEST, &PpsPeerConnection::OnRequestMsg)
MSG_MAP(PPS_MSG_PIECE, &PpsPeerConnection::OnPieceMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(PpsFsmBlockState, 0)
MSG_MAP(PPS_MSG_BITFIELD, &PpsPeerConnection::OnBitfieldMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(PpsFsmTransferState, 0)
MSG_MAP(PPS_MSG_CHOKE, &PpsPeerConnection::OnChokeMsg)
MSG_MAP(PPS_MSG_BITFIELD, &PpsPeerConnection::OnBitfieldMsg)
MSG_MAP(PPS_MSG_REQUEST, &PpsPeerConnection::OnRequestMsg)
MSG_MAP(PPS_MSG_PIECE, &PpsPeerConnection::OnPieceMsg)
MSG_MAP(PPS_MSG_EXTEND_REQUEST, &PpsPeerConnection::OnExtendedRequestMsg)
//MSG_MAP(PPS_MSG_EXTEND_PEICE, &PpsPeerConnection::OnPieceMsg)   //�ݲ���extend piece���д���
MSG_MAP_END() 

MSG_MAP_BEGIN(PpsFsmUploadState, 0)
MSG_MAP(PPS_MSG_CHOKE, &PpsPeerConnection::OnChokeMsg)
MSG_MAP(PPS_MSG_BITFIELD, &PpsPeerConnection::OnBitfieldMsg)
MSG_MAP(PPS_MSG_REQUEST, &PpsPeerConnection::OnRequestMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(PpsFsmDownloadState, 0)
MSG_MAP(PPS_MSG_CHOKE, &PpsPeerConnection::OnChokeMsg)
MSG_MAP(PPS_MSG_BITFIELD, &PpsPeerConnection::OnBitfieldMsg)
MSG_MAP(PPS_MSG_PIECE, &PpsPeerConnection::OnPieceMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(PpsFsmSeedState, 0)
MSG_MAP(PPS_MSG_CHOKE, &PpsPeerConnection::OnChokeMsg)
MSG_MAP(PPS_MSG_BITFIELD, &PpsPeerConnection::OnBitfieldMsg)
MSG_MAP(PPS_MSG_REQUEST, &PpsPeerConnection::OnRequestMsg)
MSG_MAP(PPS_MSG_PIECE, &PpsPeerConnection::OnPieceMsg)
MSG_MAP_END()

//DISPATCH_PPS_MSG(class_name, FsmMsgType, void*)
#define DISPATCH_PPS_MSG(class_name, fsm_msg_type, msg) \
if (fsm_msg_type == FSM_MSG_PKT)\
{ \
    MessageEntry* fsm_pkt_msg = reinterpret_cast<MessageEntry*>(msg); \
 \
    const char* data = fsm_pkt_msg->msg_data; \
    uint64 length = fsm_pkt_msg->msg_length; \
    PpsMsgType msg_type = pps_conn_->GetMsgType(data, length); \
    DISPATCH_MSG(class_name, *pps_conn_, msg_type, data, length); \
} \

/*-----------------------------------------------------------------------------
 * ��  ��: INIT״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmInitState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: INIT״̬����
 * ��  ��: [in] type	״̬����Ϣ����
 *         [in] msg		��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmInitState::InProc(FsmMsgType type, void* msg)
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: INIT״̬����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmInitState::ExitProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Handshake״̬ǰ����
 * ��  ��:
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.1.5
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsFsmHandshakeState::EnterProc()
{
   	if (pps_conn_->socket_connection()->connection_type() == CONN_ACTIVE)
    {
        pps_conn_->SendHandshakeMsg(false);
    }
    
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Handshake״̬����
 * ��  ��: [in]type	״̬����Ϣ����
 *         [in]msg ��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.1.5
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsFsmHandshakeState::InProc(FsmMsgType type, void* msg)
{
	if (type == FSM_MSG_TICK)
	{
		TorrentSP torrent = pps_conn_->torrent().lock();
		if (!torrent) return false;

		//LOG(INFO) << pps_conn_->handshake_complete() << " : " << torrent->is_ready();
		
		if (pps_conn_->handshake_complete() && torrent->is_ready()) //handshake��ɺ�baseinfo��ȡ��
		{
			pps_conn_->HandshakeComplete();
		}
	}
	else
	{
    	DISPATCH_PPS_MSG(PpsFsmHandshakeState, type, msg);
	}
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Handshake״̬����
 * ��  ��:
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ����tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsFsmHandshakeState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: Metadata״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��26��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmMetadataState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Metadata״̬����
 * ��  ��: [in] type ״̬����Ϣ����
 *         [in] msg	��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��26��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmMetadataState::InProc(FsmMsgType type, void* msg)
{
	if (type == FSM_MSG_TICK)
	{
		PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, pps_conn_->torrent().lock());
		if (!pps_torrent) return false;

		if (pps_conn_->socket_connection()->connection_type() == CONN_PASSIVE) return true;
	
		if (pps_torrent->metadata_ready()) //pps������is_ready, pps��metadata���߱�Ԫ����
		{
			//����Pps��û��choke��interestЭ�飬����״̬������תʵ�ִ�choke��transfer��Ҫ�յ���������Ϣ
			pps_conn_->ReceivedChokeMsg(false);
			pps_conn_->ReceivedInterestMsg(true);
			
			pps_conn_->RetrievedMetadata();
		}
	}
	else
	{
		DISPATCH_PPS_MSG(PpsFsmMetadataState, type, msg);
	}

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Metadata״̬����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��26��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmMetadataState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: Block״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmBlockState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Block״̬����
 * ��  ��: [in] type ״̬����Ϣ����
 *         [in] msg	��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmBlockState::InProc(FsmMsgType type, void* msg)
{
	// ֻ������
	DISPATCH_PPS_MSG(PpsFsmMetadataState, type, msg);

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Block״̬����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmBlockState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: Transfer״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmTransferState::EnterProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Transfer״̬����
 * ��  ��: [in] type	״̬����Ϣ����
 *         [in] msg		��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsFsmTransferState::InProc(FsmMsgType type, void* msg)
{ 
    DISPATCH_PPS_MSG(PpsFsmTransferState, type, msg);
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Transfer״̬����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmTransferState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: Upload״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmUploadState::EnterProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: INIT״̬����
 * ��  ��: [in] type	״̬����Ϣ����
 *         [in] msg		��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsFsmUploadState::InProc(FsmMsgType type, void* msg)
{
    DISPATCH_PPS_MSG(PpsFsmUploadState, type, msg);
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Upload״̬����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsFsmUploadState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: INIT״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2014.1.5
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmDownloadState::EnterProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: INIT״̬����
 * ��  ��: [in] type	״̬����Ϣ����
 *         [in] msg		��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ���� tom_liu
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool PpsFsmDownloadState::InProc(FsmMsgType type, void* msg)
{    
    DISPATCH_PPS_MSG(PpsFsmDownloadState, type, msg);
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: INIT״̬����
 * ��  ��: 
 * ����? �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2014.1.5
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmDownloadState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: Seed״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmSeedState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Seed״̬����
 * ��  ��: [in] type ״̬����Ϣ����
 *         [in] msg	��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmSeedState::InProc(FsmMsgType type, void* msg)
{
	// ֻ������
	DISPATCH_PPS_MSG(PpsFsmSeedState, type, msg);

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Seed״̬����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��11��01��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmSeedState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: Close״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmCloseState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Close״̬����
 * ��  ��: [in] type	״̬����Ϣ����
 *         [in] msg		��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmCloseState::InProc(FsmMsgType type, void* msg)
{
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Close״̬����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��09��22��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsFsmCloseState::ExitProc()
{
	return true;
}

}

