#ifndef HEADER_TRACKER_RESPONSE_MSG
#define HEADER_TRACKER_RESPONSE_MSG

#include "depend.hpp"
#include "type_def.hpp"
#include "endpoint.hpp"
#include "msg.hpp"
#include "tracker_connection.hpp"
#include "torrent.hpp"

namespace BroadCache{

// Tracker响应请求的回调处理函数
typedef boost::function<void(TrackerRequestMsg* )> TrackerResponseHandler;

class TrackerConnection;

/******************************************************************************
 * 描述：此类主要用于各协议接收来自Tracker返回消息的载体的抽象
 * 作者：tom_liu
 * 时间：2013/08/26
 *****************************************************************************/
class TrackerResponseMsg: public Msg
{
public:
	TrackerResponseMsg(Torrent& torrent):requester_(torrent){}
	virtual ~TrackerResponseMsg(){}
	char* Serialize(char * buf) const{ return buf; }
	virtual std::size_t SerializeSize() const{ return 0; }
	virtual const MsgEntry* get_msg_entry() const {return NULL;};
    virtual void  SetResponseHandler(){}
	Torrent& GetRequester() const { return requester_;}
private:
	Torrent&  requester_;
};

}

#endif

