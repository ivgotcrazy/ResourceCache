/*------------------------------------------------------------------------------
 * 文件名   : pps_fsm.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2014年1月2日
 * 文件描述 : PPS协议状态机相关类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
//MSG_MAP(PPS_MSG_EXTEND_PEICE, &PpsPeerConnection::OnPieceMsg)   //暂不对extend piece进行处理
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
 * 描  述: INIT状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月22日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsFsmInitState::EnterProc()
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
bool PpsFsmInitState::InProc(FsmMsgType type, void* msg)
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
bool PpsFsmInitState::ExitProc()
{
	return true;
}

/*------------------------------------------------------------------------------
 * 描  述: Handshake状态前处理
 * 参  数:
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.1.5
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: Handshake状态处理
 * 参  数: [in]type	状态机消息类型
 *         [in]msg 消息内容
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.1.5
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool PpsFsmHandshakeState::InProc(FsmMsgType type, void* msg)
{
	if (type == FSM_MSG_TICK)
	{
		TorrentSP torrent = pps_conn_->torrent().lock();
		if (!torrent) return false;

		//LOG(INFO) << pps_conn_->handshake_complete() << " : " << torrent->is_ready();
		
		if (pps_conn_->handshake_complete() && torrent->is_ready()) //handshake完成和baseinfo获取后
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
 * 描  述: Handshake状态后处理
 * 参  数:
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013.09.30
 *   作耿tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool PpsFsmHandshakeState::ExitProc()
{
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
bool PpsFsmMetadataState::EnterProc()
{
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
bool PpsFsmMetadataState::InProc(FsmMsgType type, void* msg)
{
	if (type == FSM_MSG_TICK)
	{
		PpsTorrentSP pps_torrent = SP_CAST(PpsTorrent, pps_conn_->torrent().lock());
		if (!pps_torrent) return false;

		if (pps_conn_->socket_connection()->connection_type() == CONN_PASSIVE) return true;
	
		if (pps_torrent->metadata_ready()) //pps不能用is_ready, pps的metadata不具备元数据
		{
			//由于Pps中没有choke和interest协议，对于状态机的跳转实现从choke到transfer需要收到这两个消息
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
 * 描  述: Metadata状态后处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2013年09月26日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsFsmMetadataState::ExitProc()
{
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
bool PpsFsmBlockState::EnterProc()
{
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
bool PpsFsmBlockState::InProc(FsmMsgType type, void* msg)
{
	// 只处理报文
	DISPATCH_PPS_MSG(PpsFsmMetadataState, type, msg);

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
bool PpsFsmBlockState::ExitProc()
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
bool PpsFsmTransferState::EnterProc()
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
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool PpsFsmTransferState::InProc(FsmMsgType type, void* msg)
{ 
    DISPATCH_PPS_MSG(PpsFsmTransferState, type, msg);
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
bool PpsFsmTransferState::ExitProc()
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
bool PpsFsmUploadState::EnterProc()
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
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool PpsFsmUploadState::InProc(FsmMsgType type, void* msg)
{
    DISPATCH_PPS_MSG(PpsFsmUploadState, type, msg);
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
bool PpsFsmUploadState::ExitProc()
{
	return true;
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: INIT状态前处理
 * 参  数: 
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2014.1.5
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsFsmDownloadState::EnterProc()
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
 *   作者 tom_liu
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool PpsFsmDownloadState::InProc(FsmMsgType type, void* msg)
{    
    DISPATCH_PPS_MSG(PpsFsmDownloadState, type, msg);
	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: INIT状态后处理
 * 参  数: 
 * 祷刂? 成功/失败
 * 修  改:
 *   时间 2014.1.5
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsFsmDownloadState::ExitProc()
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
bool PpsFsmSeedState::EnterProc()
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
bool PpsFsmSeedState::InProc(FsmMsgType type, void* msg)
{
	// 只处理报文
	DISPATCH_PPS_MSG(PpsFsmSeedState, type, msg);

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
bool PpsFsmSeedState::ExitProc()
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
bool PpsFsmCloseState::EnterProc()
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
bool PpsFsmCloseState::InProc(FsmMsgType type, void* msg)
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
bool PpsFsmCloseState::ExitProc()
{
	return true;
}

}

