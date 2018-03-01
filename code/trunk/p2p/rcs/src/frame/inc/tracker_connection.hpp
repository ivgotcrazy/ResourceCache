/*#############################################################################
 * �ļ���   : tracker_connection.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013-8-19
 * �ļ����� : tracker_conneciton  ͷ�ļ�����
 * ##########################################################################*/
 
#ifndef  HEADER_TRACKER_CONNECTION
#define  HEADER_TRACKER_CONNECTION

#include "depend.hpp"
#include "type_def.hpp"
#include "endpoint.hpp"
#include "tracker_manager.hpp"
#include "torrent.hpp"

namespace BroadCache
{

class TrackerManager;

/******************************************************************************
 * ������������Ҫ���ڸ�Э�鷢�����ӵ�����࣬�����˴��µ����ӹ���
 * ���ߣ�tom_liu
 * ʱ�䣺2013/08/26
 *****************************************************************************/
class TrackerConnection
{
public:
	TrackerConnection(TrackerManager& man, boost::asio::io_service& ios, EndPoint& ep, Torrent& torrent);
	virtual ~TrackerConnection(){}
	virtual void Lanuch() = 0;
	virtual void Close() = 0;
	Torrent& GetRequester() const;

protected:
	TrackerManager& manager() {return manager_;};
	boost::asio::io_service& ios() {return ios_;}
	EndPoint& endpoint() {return end_point_;}
	Torrent&  torrent() {return torrent_;}
		
private:
	TrackerManager& manager_;
	boost::asio::io_service& ios_;
	EndPoint&   end_point_;
	Torrent&	torrent_;
	
};

}
#endif
