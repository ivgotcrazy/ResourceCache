/*#############################################################################
 * 文件名   : bt_tracker_request.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月30日
 * 文件描述 : BtTrackerRequest声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "endpoint.hpp"
#include "tracker_request.hpp"

namespace BroadCache
{

class MemBufferPool;
class LazyEntry;

/******************************************************************************
 * 描述：BT协议的Announce请求
 * 作者：teck_zhou
 * 时间：2013/10/30
 *****************************************************************************/
class BtTrackerRequest : public TrackerRequest
{
public:
	BtTrackerRequest(Session& session, const TorrentSP& torrent, 
		             const EndPoint& tracker_address);

private:
	virtual SocketConnectionSP CreateConnection() override;
	virtual bool ConstructSendRequest(SendBuffer& send_buf) override;
	virtual bool ProcessResponse(const char* buf, uint64 len) override;

	bool ParseTrackerResponse(const std::string& response);
	bool Parse(const LazyEntry& e, std::vector<EndPoint>& peer_list);
	bool ExtractPeerInfo(const LazyEntry& info, EndPoint& peer);

	void AddPeers(std::vector<EndPoint> peers);

private:
	Session& session_;
	TorrentWP torrent_;
	EndPoint local_address_;
};

}