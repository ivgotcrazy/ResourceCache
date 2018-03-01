/*------------------------------------------------------------------------------
 * �ļ���   : bt_fsm.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��16��
 * �ļ����� : BTЭ��״̬�������ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/

#include "bt_fsm.hpp"
#include "bt_metadata_retriver.hpp"
#include "torrent.hpp"
#include "info_hash.hpp"
#include "peer_connection.hpp"
#include "bt_peer_connection.hpp"
#include "bt_torrent.hpp"
#include "mem_buffer_pool.hpp"
#include "socket_connection.hpp"
#include "session.hpp"
#include "rcs_config.hpp"
#include "bc_util.hpp"
#include "protocol_msg_macro.hpp"

namespace BroadCache
{

MSG_MAP_BEGIN(BtFsmHandshakeState, 0)
MSG_MAP(BT_MSG_HANDSHAKE, &BtPeerConnection::OnHandshakeMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(BtFsmMetadataState, 0)
MSG_MAP(BT_MSG_CHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_UNCHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_INTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_NOTINTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_HAVE, &BtPeerConnection::OnHaveMsg)
MSG_MAP(BT_MSG_BITFIELD, &BtPeerConnection::OnBitfieldMsg)
MSG_MAP(BT_MSG_EXTEND_ANNOUNCE, &BtPeerConnection::OnExtendAnnounceMsg)
MSG_MAP(BT_MSG_METADATA_REQUEST, &BtPeerConnection::OnMetadataRequestMsg)
MSG_MAP(BT_MSG_METADATA_REQUEST, &BtPeerConnection::OnMetadataRequestMsg)
MSG_MAP(BT_MSG_METADATA_DATA, &BtPeerConnection::OnMetadataDataMsg)
MSG_MAP(BT_MSG_METADATA_REJECT, &BtPeerConnection::OnMetadataRejectMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(BtFsmBlockState, 0)
MSG_MAP(BT_MSG_CHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_UNCHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_INTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_NOTINTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_HAVE, &BtPeerConnection::OnHaveMsg)
MSG_MAP(BT_MSG_BITFIELD, &BtPeerConnection::OnBitfieldMsg)
MSG_MAP(BT_MSG_METADATA_REQUEST, &BtPeerConnection::OnMetadataRequestMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(BtFsmTransferState, 0)
MSG_MAP(BT_MSG_CHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_UNCHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_INTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_NOTINTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_HAVE, &BtPeerConnection::OnHaveMsg)
MSG_MAP(BT_MSG_BITFIELD, &BtPeerConnection::OnBitfieldMsg)
MSG_MAP(BT_MSG_REQUEST, &BtPeerConnection::OnRequestMsg)
MSG_MAP(BT_MSG_PIECE, &BtPeerConnection::OnPieceMsg)
MSG_MAP(BT_MSG_METADATA_REQUEST, &BtPeerConnection::OnMetadataRequestMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(BtFsmUploadState, 0)
MSG_MAP(BT_MSG_CHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_UNCHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_INTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_NOTINTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_HAVE, &BtPeerConnection::OnHaveMsg)
MSG_MAP(BT_MSG_BITFIELD, &BtPeerConnection::OnBitfieldMsg)
MSG_MAP(BT_MSG_REQUEST, &BtPeerConnection::OnRequestMsg)
MSG_MAP(BT_MSG_METADATA_REQUEST, &BtPeerConnection::OnMetadataRequestMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(BtFsmDownloadState, 0)
MSG_MAP(BT_MSG_CHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_UNCHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_INTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_NOTINTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_HAVE, &BtPeerConnection::OnHaveMsg)
MSG_MAP(BT_MSG_BITFIELD, &BtPeerConnection::OnBitfieldMsg)
MSG_MAP(BT_MSG_PIECE, &BtPeerConnection::OnPieceMsg)
MSG_MAP(BT_MSG_METADATA_REQUEST, &BtPeerConnection::OnMetadataRequestMsg)
MSG_MAP_END()

MSG_MAP_BEGIN(BtFsmSeedState, 0)
MSG_MAP(BT_MSG_CHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_UNCHOKE, &BtPeerConnection::OnChokeMsg)
MSG_MAP(BT_MSG_INTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_NOTINTERESTED, &BtPeerConnection::OnInterestedMsg)
MSG_MAP(BT_MSG_HAVE, &BtPeerConnection::OnHaveMsg)
MSG_MAP(BT_MSG_BITFIELD, &BtPeerConnection::OnBitfieldMsg)
MSG_MAP(BT_MSG_REQUEST, &BtPeerConnection::OnRequestMsg)
MSG_MAP(BT_MSG_PIECE, &BtPeerConnection::OnPieceMsg)
MSG_MAP(BT_MSG_METADATA_REQUEST, &BtPeerConnection::OnMetadataRequestMsg)
MSG_MAP_END()

//DISPATCH_BT_MSG(class_name, FsmMsgType, void*)
#define DISPATCH_BT_MSG(class_name, fsm_msg_type, msg) \
if (fsm_msg_type == FSM_MSG_PKT)\
{ \
    MessageEntry* fsm_pkt_msg = reinterpret_cast<MessageEntry*>(msg); \
 \
    const char* data = fsm_pkt_msg->msg_data; \
    uint64 length = fsm_pkt_msg->msg_length; \
    BtMsgType msg_type = bt_conn_->GetMsgType(data, length); \
    DISPATCH_MSG(class_name, *bt_conn_, msg_type, data, length); \
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
bool BtFsmInitState::EnterProc()
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
bool BtFsmInitState::InProc(FsmMsgType type, void* msg)
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
bool BtFsmInitState::ExitProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Handshake״̬ǰ����
 * ��  ��:
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.09.27
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtFsmHandshakeState::EnterProc()
{
    bt_conn_->SendHandshakeMsg();
    
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Handshake״̬����
 * ��  ��: [in]type	״̬����Ϣ����
 *         [in]msg ��Ϣ����
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.09.27
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtFsmHandshakeState::InProc(FsmMsgType type, void* msg)
{
    DISPATCH_BT_MSG(BtFsmHandshakeState, type, msg);
	return true;
}

/*------------------------------------------------------------------------------
 * ��  ��: Handshake״̬����
 * ��  ��:
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013.09.30
 *   ����rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtFsmHandshakeState::ExitProc()
{
	TorrentSP torrent = peer_connection()->torrent().lock();
	if (!torrent) return true;

	if (torrent->is_ready())
	{
        bt_conn_->SendBitfieldMsg();
    }

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
bool BtFsmMetadataState::EnterProc()
{
	// �����Ȳ���ʲô�����������չ������Ϣ
	bt_conn_->SendExtendAnnounceMsg();

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
bool BtFsmMetadataState::InProc(FsmMsgType type, void* msg)
{
	if (type == FSM_MSG_TICK)
	{
		TorrentSP torrent = bt_conn_->torrent().lock();
		if (!torrent) return false;

		if (torrent->is_ready())
		{
			bt_conn_->RetrievedMetadata();
		}
	}
	else
	{
		DISPATCH_BT_MSG(BtFsmMetadataState, type, msg);
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
bool BtFsmMetadataState::ExitProc()
{
	TorrentSP torrent = peer_connection()->torrent().lock();
	if (!torrent) return true;

	// ��ȡ��metadata�󣬿��Է���������metadata��չ��Ϣ��
	bt_conn_->SendExtendAnnounceMsg();

	if (torrent->is_ready())
	{
		bt_conn_->SendBitfieldMsg();
	}

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
bool BtFsmBlockState::EnterProc()
{
	bt_conn_->SendChokeMsg(false);
	bt_conn_->SendInterestMsg(true);

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
bool BtFsmBlockState::InProc(FsmMsgType type, void* msg)
{
	// ֻ������
	DISPATCH_BT_MSG(BtFsmMetadataState, type, msg);

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
bool BtFsmBlockState::ExitProc()
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
bool BtFsmTransferState::EnterProc()
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
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtFsmTransferState::InProc(FsmMsgType type, void* msg)
{ 
    DISPATCH_BT_MSG(BtFsmTransferState, type, msg);
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
bool BtFsmTransferState::ExitProc()
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
bool BtFsmUploadState::EnterProc()
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
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtFsmUploadState::InProc(FsmMsgType type, void* msg)
{
    DISPATCH_BT_MSG(BtFsmUploadState, type, msg);
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
bool BtFsmUploadState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: INIT״̬ǰ����
 * ��  ��: 
 * ����ֵ: �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��XX��XX��
 *   ���� XXX
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtFsmDownloadState::EnterProc()
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
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
bool BtFsmDownloadState::InProc(FsmMsgType type, void* msg)
{    
    DISPATCH_BT_MSG(BtFsmDownloadState, type, msg);
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: INIT״̬����
 * ��  ��: 
 * ����? �ɹ�/ʧ��
 * ��  ��:
 *   ʱ�� 2013��XX��XX��
 *   ���� XXX
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtFsmDownloadState::ExitProc()
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
bool BtFsmSeedState::EnterProc()
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
bool BtFsmSeedState::InProc(FsmMsgType type, void* msg)
{
	// ֻ������
	DISPATCH_BT_MSG(BtFsmSeedState, type, msg);

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
bool BtFsmSeedState::ExitProc()
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
bool BtFsmCloseState::EnterProc()
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
bool BtFsmCloseState::InProc(FsmMsgType type, void* msg)
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
bool BtFsmCloseState::ExitProc()
{
	return true;
}

}
