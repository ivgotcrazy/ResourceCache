#include "inc/tracker_connection.hpp"
#include "inc/tracker_manager.hpp"


namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * 描  述: TrackerConnection构造函数
 * 参  数:[in] TrackerManager& man 
 *	     [in] io_service&  ios
 *        [in]	 EndPoint& ep	   
 * 返回值:
 * 修  改:
 * 时间 2013年08月26日
 * 作者 tom_liu
 * 描述 创建
 ----------------------------------------------------------------------------*/
TrackerConnection::TrackerConnection(TrackerManager &man,
    boost::asio::io_service& ios,EndPoint& ep,
    Torrent& torrent):
	manager_(man),
	ios_(ios),
	end_point_(ep),
	torrent_(torrent)
{	
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取torrent引用,获得请求者的引用 
 * 参  数:
 * 返回值: 获取torrent引用
 * 修  改: 
 * 时间 2013年08月26日
 * 作者 tom_liu
 * 描述 创建
 ----------------------------------------------------------------------------*/
Torrent& TrackerConnection::GetRequester() const
{ 
	return  torrent_;
}

}
