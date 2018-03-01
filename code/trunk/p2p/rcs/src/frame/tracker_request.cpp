/*#############################################################################
 * 文件名   : tracker_request.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月29日
 * 文件描述 : TrackerRequest类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "tracker_request.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"
#include "socket_connection.hpp"
#include "tracker_request_manager.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 由于是异步处理，因此在析构之前需要先将socket关闭，以免TrackerRequest
 *         析构后，sock_conn_未析构，产生异步回调出现问题
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
TrackerRequest::~TrackerRequest() 
{
	LOG(INFO) << ">>> Destructing tracker request";
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理请求
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequest::Process()
{
	LOG(INFO) << "Begin to process tracker request | " << tracker_address_;

	sock_conn_ = CreateConnection();
	BC_ASSERT(sock_conn_);

	TrackerRequestWP weak_request = shared_from_this();

	sock_conn_->set_connect_handler(boost::bind(
		&TrackerRequest::OnConnect, this, weak_request, _1));

	sock_conn_->set_recv_handler(boost::bind(
		&TrackerRequest::OnReceiveData, this, weak_request, _1, _2, _3));

	sock_conn_->Connect();
}

/*-----------------------------------------------------------------------------
 * 描  述: 秒任务处理
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月16日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequest::OnTick()
{
	TickProc();

	if (IsTimeout()) FinishRequest();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取本端Endpoint
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月16日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
EndPoint TrackerRequest::GetLocalAddress() const
{
	return sock_conn_->connection_id().local;	
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理连接
 * 参  数: [in] error 错误码
 * 返回值:
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequest::OnConnect(const TrackerRequestWP& weak_request, ConnectionError error)
{
	TrackerRequestSP request = weak_request.lock();
	if (!request) return;

	if (error != CONN_ERR_SUCCESS)
	{
		LOG(ERROR) << "Fail to connect tracker | " << tracker_address_;
		return FinishRequest();
	}

	BC_ASSERT(sock_conn_->is_connected());

	SendRequest();
}

/*-----------------------------------------------------------------------------
 * 描  述: 发送请求
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月16日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequest::SendRequest()
{
	SendBuffer request;
	if (!ConstructSendRequest(request))
	{
		LOG(WARNING) << "Fail to construct tracker request";
		return FinishRequest();
	}

	sock_conn_->SendData(request);
}

/*-----------------------------------------------------------------------------
 * 描  述: 接收数据处理
 * 参  数: [in] error 错误码
 *         [in] buf 接收数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 * 时间 2013年10月30日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequest::OnReceiveData(const TrackerRequestWP& weak_request, 
	                               ConnectionError error, 
								   const char* buf, 
								   uint64 len)
{
	TrackerRequestSP request = weak_request.lock();
	if (!request) return;

	if (error != CONN_ERR_SUCCESS)
	{
		LOG(ERROR) << "Fail to get response from tracker " << tracker_address_;
	}
	else
	{
		// 处理需要继续发送请求情况
		if (ProcessResponse(buf, len)) return SendRequest();
	}

	FinishRequest();
}

/*-----------------------------------------------------------------------------
 * 描  述: 结束请求
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月16日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequest::FinishRequest()
{
	TrackerRequestManager& request_manager = TrackerRequestManager::GetInstance();
	request_manager.OnRequestProcessed(shared_from_this());
}

}
