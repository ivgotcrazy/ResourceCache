#ifndef HEADER_TRACKER_RESPONSE_MSG
#define HEADER_TRACKER_RESPONSE_MSG

#include "depend.hpp"
#include "type_def.hpp"
#include "endpoint.hpp"
#include "msg.hpp"
#include "tracker_connection.hpp"
#include "torrent.hpp"

namespace BroadCache{

// Tracker��Ӧ����Ļص�������
typedef boost::function<void(TrackerRequestMsg* )> TrackerResponseHandler;

class TrackerConnection;

/******************************************************************************
 * ������������Ҫ���ڸ�Э���������Tracker������Ϣ������ĳ���
 * ���ߣ�tom_liu
 * ʱ�䣺2013/08/26
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

