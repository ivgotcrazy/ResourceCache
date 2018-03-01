/*#############################################################################
 * �ļ���   : bt_tracker_request.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��30��
 * �ļ����� : BtTrackerRequest����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ������BTЭ���Announce����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/30
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