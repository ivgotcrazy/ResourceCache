#include "inc/tracker_connection.hpp"
#include "inc/tracker_manager.hpp"


namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * ��  ��: TrackerConnection���캯��
 * ��  ��:[in] TrackerManager& man 
 *	     [in] io_service&  ios
 *        [in]	 EndPoint& ep	   
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��08��26��
 * ���� tom_liu
 * ���� ����
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
 * ��  ��: ��ȡtorrent����,��������ߵ����� 
 * ��  ��:
 * ����ֵ: ��ȡtorrent����
 * ��  ��: 
 * ʱ�� 2013��08��26��
 * ���� tom_liu
 * ���� ����
 ----------------------------------------------------------------------------*/
Torrent& TrackerConnection::GetRequester() const
{ 
	return  torrent_;
}

}
