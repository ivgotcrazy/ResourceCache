/*#############################################################################
 * 文件名   : tracker_request.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月29日
 * 文件描述 : TrackerRequest类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef TRACKER_REQUEST_HEAD
#define TRACKER_REQUEST_HEAD

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "connection.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述：所有tracker请求都应该从这个类派生
 * 作者：teck_zhou
 * 时间：2013年10月30日
 *****************************************************************************/
class TrackerRequest : public boost::noncopyable,
	                   public boost::enable_shared_from_this<TrackerRequest>
{
public:
	TrackerRequest(const EndPoint& address) : tracker_address_(address) {}
	virtual ~TrackerRequest();

	void Process();

	void OnTick();

	EndPoint tracker_address() const { return tracker_address_; }

protected:
	EndPoint GetLocalAddress() const;

private:

	// 创建连接，支持UDP和TCP
	virtual SocketConnectionSP CreateConnection() = 0;

	// 构造待发送请求，如果设计多次协议交互，派生类内部负责记录状态
	virtual bool ConstructSendRequest(SendBuffer& send_buf) = 0;

	// 返回值表示是否需要继续发送报文，true: 是，false: 否
	virtual bool ProcessResponse(const char* buf, uint64 len) = 0;

	// 如果派生类希望自己控制超时处理，则可以实现以下两个接口
	virtual void TickProc() { return; }
	virtual bool IsTimeout() { return false; }
	
private:
	void OnConnect(const TrackerRequestWP& weak_request, ConnectionError error);
	void OnReceiveData(const TrackerRequestWP& weak_request, ConnectionError error, const char* buf, uint64 len);
	void SendRequest();
	void FinishRequest();

private:
	EndPoint tracker_address_;
	SocketConnectionSP sock_conn_;
};

}

#endif