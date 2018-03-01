/*#############################################################################
 * 文件名   : bt_udp_tracker_request.hpp
 * 创建人   : vincent_pan
 * 创建时间 : 2013年11月18日
 * 文件描述 : BtUdpTrackerRequest声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef BT_UDP_TRACKER_REQUEST_HEAD
#define BT_UDP_TRACKER_REQUEST_HEAD

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "tracker_request.hpp"

#define TIMEOUT  15     //超时时间

namespace BroadCache
{
/*
struct TConnectPacket      // connect 报文的结构
{
	int64 connection_id;
	int32 action_id;
	int32 transaction_id;
};

struct TAnnouncPacket      // announce 报文的结构
{
	int64 connection_id;
	int32 action_id;
	int32 transaction_id;	
	char info_hash[20];
	char peer_id[20];
	int64 downloaded;
	int64 left;
	int64 uploaded;
	int32 event;
	uint32 ip;
	uint32 key;
	int32 num_want;
	uint16 port; 
	uint16 extensions;
}; */

enum State
{
	TS_INIT = 1,
	TS_CONNECTING = 2,
	TS_ANNOUNCING = 3,
	TS_OK = 4,
	TS_ERROR = 5
};

/******************************************************************************
 * 描述: udp traker 请求类  BtUdpTrackerRequest
 * 作者：vicent_pan
 * 时间：2013/11/18
 *****************************************************************************/
class BtUdpTrackerRequest: public TrackerRequest
{
public:
	BtUdpTrackerRequest(Session & session, const TorrentSP & torrent, const EndPoint & traker_address);

private:
	virtual SocketConnectionSP CreateConnection() override;
	virtual bool ConstructSendRequest(SendBuffer& send_buf) override;
	virtual bool ProcessResponse(const char* buf, uint64 len) override;
	virtual void TickProc() override;
	virtual bool IsTimeout() override;

private:
	bool ProcessPkg(uint32 action_id, const char* buf, uint64 len);
	void SendConnectRequest(SendBuffer& send_buf);
	bool SendAnnounceRequest(SendBuffer& send_buf);

private:
	State state_;
	EndPoint local_address_;
	TorrentWP torrent_;
	Session & session_;
	int64 connection_id_;
	uint32 transaction_id_;
	uint32 alive_time_;
	uint32 send_time_;
};

}
#endif 