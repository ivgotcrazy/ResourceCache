/*#############################################################################
 * 文件名   : pps_baseinfo_tracker_request.cpp
 * 创建人   : tom_liu
 * 创建时间 : 2014年1月2日
 * 文件描述 : PpsBaseInfoTrackerRequest实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#include "pps_baseinfo_tracker_request.hpp"

#include "net_byte_order.hpp"
#include "pps_session.hpp"
#include "torrent.hpp"
#include "policy.hpp"
#include "ip_filter.hpp"
#include "pps_pub.hpp"
#include "bc_io.hpp"
#include "rcs_util.hpp"
#include "pps_torrent.hpp"
#include "pps_baseinfo_retriver.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
PpsBaseinfoTrackerRequest::PpsBaseinfoTrackerRequest(Session& session,
																const TorrentSP& torrent,
																const PpsBaseinfoRetriverSP& retriver, 
															  	const EndPoint & traker_address, 
																TimeoutHandler handler)
	: TrackerRequest(traker_address)
	, session_(session)
	, torrent_(torrent)
	, baseinfo_retriver_(retriver)
	, alive_time_(0)
	, send_time_(0)
{

}

/*-----------------------------------------------------------------------------
 * 描  述: 创建socket 连接
 * 参  数:
 * 返回值: SocketConnectionSP
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
SocketConnectionSP PpsBaseinfoTrackerRequest::CreateConnection()
{
	SocketConnectionSP sock_conn (new UdpSocketConnection(session_.sock_ios(), tracker_address()));
	return sock_conn;
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造send request 报文
 * 参  数: [out] send_buf   发送报文缓存区
 * 返回值: bool true 构建成功，false 失败
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoTrackerRequest::ConstructSendRequest(SendBuffer& send_buf)
{	
	SendBaseinfoRequest(send_buf);

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述:构建 baseinfo 请求报文 
 * 参  数:[out] send_buf
 * 返回值: bool ture: 构建成功 false : 构建失败
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoTrackerRequest::SendBaseinfoRequest(SendBuffer& send_buf)
{
	TorrentSP torrent = torrent_.lock();
	if (!torrent) return false;

	LOG(INFO) << "Send Pps baseinfo request msg";
	
	send_buf = session_.mem_buf_pool().AllocMsgBuffer(MSG_BUF_COMMON);

	char* buffer = send_buf.buf;
	buffer += 2;  //先不计算packet_len

	IO::write_uint8(kProtoPps, buffer); //标识字段
	IO::write_uint32(PPS_MSG_INFO_TRACKER_REQ, buffer);

	IO::write_uint8(0x14, buffer);
	std::memcpy(buffer, torrent->info_hash()->raw_data().c_str(), 20); //填充20位的info-hash
	buffer += 20;

	IO::write_uint32(0x00, buffer);
	IO::write_uint32_be(0x09, buffer);
	IO::write_uint16(0x00, buffer);

	//对于标准的baseinfo请求消息还要加上 pps_link_len, pps_link两个字段，
	//经测试不加此两字段同样可以达到获取metadata_info的效果
	uint16 packet_len = buffer - send_buf.buf; //metadata不用减4 
	char* front_pos = send_buf.buf;
	IO::write_uint16_be(packet_len, front_pos); //不能直接用send_buf.buf,write系列函数会对buffer移位
	
	send_buf.len = packet_len;

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 接收报文的解析接口
 * 参  数: [in] buf 接收缓冲区 
           [in] len 缓冲区大小
 * 返回值: bool true: 继续发送报文 false 不继续发送报文
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoTrackerRequest::ProcessResponse(const char* buf, uint64 len)
{	
	LOG(INFO) << "Recv tracker response ";
	//截取长度之后的3字节，匹配标识字段
	const char* buffer;
	buffer = buf + 3;
	PpsMsgType msg_type = static_cast<PpsMsgType>(IO::read_uint32(buffer));

	if (msg_type != PPS_MSG_INFO_TRACKER_RES) return false;

	ProcBaseinfoResp(buf, len);

	return false;
}

/*-----------------------------------------------------------------------------
 * 描  述: 解析Baseinfo消息
 * 参  数: [in] buf 接收缓冲区 
 *         [in] len 缓冲区大小
 *         [out] msg baseinfo消息 
 * 返回值: 
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsBaseinfoTrackerRequest::ParseBaseinfoResp(const char* buf, uint64 len, PpsBaseInfoMsg& msg)
{
	buf += 2; //packet_len
	buf += 5; //identify_id	
	buf += 1 + 20; // hash_len + hash  
	buf += 1 + 20; //hash_len + hash

	msg.bitfield_size = IO::read_uint32_be(buf);

	buf += 4; // unknown

	msg.piece_count = IO::read_uint32_be(buf);

	buf += 4; //mark
	buf += 4; //unknown
	buf += 4; //crc

	msg.metadata_size = IO::read_uint32_be(buf);

	buf += 4; //bitfiled checksum

	msg.file_size = IO::read_uint32_be(buf);

	//剩余31未知字节暂未发现影响交互，暂不处理
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理Baseinfo回应
 * 参  数: [in] buf 接收缓冲区 
 *         [in] len 缓冲区大小
 * 返回值: 
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsBaseinfoTrackerRequest::ProcBaseinfoResp(const char* buf, uint64 len)
{
	LOG(INFO) << "Proc base info response ";
	PpsBaseInfoMsg msg;
	ParseBaseinfoResp(buf, len, msg);
	
	//加载到baseinfo到PpsBaseinfoRetriver中
	baseinfo_retriver_->AddBaseInfo(msg);
}

/*-----------------------------------------------------------------------------
 * 描  述: tick 处理接口
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PpsBaseinfoTrackerRequest::TickProc()
{
	if (IsTimeout())
	{
		TimeoutHandler();		
	}
	
	alive_time_++;     
}

/*-----------------------------------------------------------------------------
 * 描  述: 是否超时
 * 参  数:
 * 返回值: bool true: 是超时 false :没有超时
 * 修  改:
 *   时间 2014年1月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool PpsBaseinfoTrackerRequest::IsTimeout()
{	
	if (!send_time_ && (alive_time_ - send_time_ >= TIMEOUT )) return true;
	return false;
}
	
}

