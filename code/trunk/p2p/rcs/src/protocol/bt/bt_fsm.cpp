/*------------------------------------------------------------------------------
 * 文件名   : bt_fsm.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月16日
 * 文件描述 : BT协议状态机相关类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描  述: INIT状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmInitState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: INIT状态处理
 * 参  数: [in] type	状态机消息类型
 *         [in] msg		消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmInitState::InProc(FsmMsgType type, void* msg)
{
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: INIT状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmInitState::ExitProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: Handshake状态前处理
 * 参  数:
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.09.27
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtFsmHandshakeState::EnterProc()
{
    bt_conn_->SendHandshakeMsg();
    
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: Handshake状态处理
 * 参  数: [in]type	状态机消息类型
 *         [in]msg 消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.09.27
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtFsmHandshakeState::InProc(FsmMsgType type, void* msg)
{
    DISPATCH_BT_MSG(BtFsmHandshakeState, type, msg);
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: Handshake状态后处理
 * 参  数:
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.09.30
 *   作耿rosan
 *   描述 创建
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
 * 描  述: Metadata状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月26日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmMetadataState::EnterProc()
{
	// 这里先不管什么情况都发送扩展宣告消息
	bt_conn_->SendExtendAnnounceMsg();

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Metadata状态处理
 * 参  数: [in] type 状态机消息类型
 *         [in] msg	消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月26日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: Metadata状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月26日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmMetadataState::ExitProc()
{
	TorrentSP torrent = peer_connection()->torrent().lock();
	if (!torrent) return true;

	// 获取完metadata后，可以发送完整的metadata扩展消息了
	bt_conn_->SendExtendAnnounceMsg();

	if (torrent->is_ready())
	{
		bt_conn_->SendBitfieldMsg();
	}

	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: Block状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmBlockState::EnterProc()
{
	bt_conn_->SendChokeMsg(false);
	bt_conn_->SendInterestMsg(true);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Block状态处理
 * 参  数: [in] type 状态机消息类型
 *         [in] msg	消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmBlockState::InProc(FsmMsgType type, void* msg)
{
	// 只处理报文
	DISPATCH_BT_MSG(BtFsmMetadataState, type, msg);

    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Block状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmBlockState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: Transfer状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmTransferState::EnterProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: Transfer状态处理
 * 参  数: [in] type	状态机消息类型
 *         [in] msg		消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.09.30
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtFsmTransferState::InProc(FsmMsgType type, void* msg)
{ 
    DISPATCH_BT_MSG(BtFsmTransferState, type, msg);
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Transfer状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmTransferState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: Upload状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmUploadState::EnterProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: INIT状态处理
 * 参  数: [in] type	状态机消息类型
 *         [in] msg		消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.09.30
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtFsmUploadState::InProc(FsmMsgType type, void* msg)
{
    DISPATCH_BT_MSG(BtFsmUploadState, type, msg);
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: Upload状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtFsmUploadState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: INIT状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年XX月XX日
 *   作者 XXX
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmDownloadState::EnterProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: INIT状态处理
 * 参  数: [in] type	状态机消息类型
 *         [in] msg		消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.09.30
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool BtFsmDownloadState::InProc(FsmMsgType type, void* msg)
{    
    DISPATCH_BT_MSG(BtFsmDownloadState, type, msg);
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: INIT状态后处理
 * 参  数: 
 * 祷刂? 成功/失败
 * 修  改:
 *   时间 2013年XX月XX日
 *   作者 XXX
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmDownloadState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: Seed状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmSeedState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Seed状态处理
 * 参  数: [in] type 状态机消息类型
 *         [in] msg	消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmSeedState::InProc(FsmMsgType type, void* msg)
{
	// 只处理报文
	DISPATCH_BT_MSG(BtFsmSeedState, type, msg);

    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Seed状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年11月01日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmSeedState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: Close状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmCloseState::EnterProc()
{
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Close状态处理
 * 参  数: [in] type	状态机消息类型
 *         [in] msg		消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmCloseState::InProc(FsmMsgType type, void* msg)
{
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: Close状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool BtFsmCloseState::ExitProc()
{
	return true;
}

}
