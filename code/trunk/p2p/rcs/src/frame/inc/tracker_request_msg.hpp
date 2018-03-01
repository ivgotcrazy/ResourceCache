#ifndef HEADER_TRACKER_REQUEST_MSG
#define HEADER_TRACKER_REQUEST_MSG

#include "depend.hpp"
#include "type_def.hpp"
#include "endpoint.hpp"
#include "msg.hpp"
#include "tracker_manager.hpp"
#include "tracker_connection.hpp"
#include "torrent.hpp"

namespace BroadCache{

class TrackerConnection;
class TrackerManager;

/******************************************************************************
 * 描述：此类主要用于各协议发往Tracker的消息的载体的抽象
 * 作者：tom_liu
 * 时间：2013/08/26
 *****************************************************************************/
class TrackerRequestMsg: public Msg
{
public:
	TrackerRequestMsg(TrackerManager& man, boost::asio::io_service& ios, 
		Torrent& tt):
		man_(man),
		ios_(ios),
		torrent_(tt)
	{}
	virtual ~TrackerRequestMsg(){}
	const char* Unserialize(const char * buf){ return buf; }
	const MsgEntry* get_msg_entry() const { return NULL; }
	virtual TrackerConnection*  CreateTrackerConnection() = 0 ;
	virtual EndPoint&  GetEndPoint() = 0;
	
protected:
	TrackerManager& man_;
	boost::asio::io_service& ios_;
	Torrent& torrent_;
	std::size_t  buf_length_;
};




}

#endif

