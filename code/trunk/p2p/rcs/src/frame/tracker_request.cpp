/*#############################################################################
 * �ļ���   : tracker_request.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��29��
 * �ļ����� : TrackerRequest��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "tracker_request.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"
#include "socket_connection.hpp"
#include "tracker_request_manager.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: �������첽�������������֮ǰ��Ҫ�Ƚ�socket�رգ�����TrackerRequest
 *         ������sock_conn_δ�����������첽�ص���������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
TrackerRequest::~TrackerRequest() 
{
	LOG(INFO) << ">>> Destructing tracker request";
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
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
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��16��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
void TrackerRequest::OnTick()
{
	TickProc();

	if (IsTimeout()) FinishRequest();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����Endpoint
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��16��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
EndPoint TrackerRequest::GetLocalAddress() const
{
	return sock_conn_->connection_id().local;	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: [in] error ������
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
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
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��16��
 * ���� teck_zhou
 * ���� ����
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
 * ��  ��: �������ݴ���
 * ��  ��: [in] error ������
 *         [in] buf ��������
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��30��
 * ���� teck_zhou
 * ���� ����
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
		// ������Ҫ���������������
		if (ProcessResponse(buf, len)) return SendRequest();
	}

	FinishRequest();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��16��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
void TrackerRequest::FinishRequest()
{
	TrackerRequestManager& request_manager = TrackerRequestManager::GetInstance();
	request_manager.OnRequestProcessed(shared_from_this());
}

}
