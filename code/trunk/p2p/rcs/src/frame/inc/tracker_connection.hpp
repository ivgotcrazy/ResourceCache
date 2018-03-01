/*#############################################################################
 * 文件名   : tracker_connection.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013-8-19
 * 文件描述 : tracker_conneciton  头文件声明
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
 * 描述：此类主要用于各协议发起连接的虚基类，抽象了大致的连接过程
 * 作者：tom_liu
 * 时间：2013/08/26
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
