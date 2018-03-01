/*#############################################################################
 * �ļ���   : tracker_request_manager.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��30��
 * �ļ����� : TrackerRequestManagerʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "tracker_request_manager.hpp"
#include "tracker_request.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��25��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
TrackerRequestManager& TrackerRequestManager::GetInstance()
{
	static TrackerRequestManager manager;
	return manager;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��03��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
TrackerRequestManager::TrackerRequestManager() : stop_flag_(false)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��03��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
TrackerRequestManager::~TrackerRequestManager()
{
	stop_flag_ = true;

	thread_->join();
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����߳�
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��02��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
void TrackerRequestManager::Init(io_service& ios)
{
	request_timer_.reset(new Timer(ios, boost::bind(&TrackerRequestManager::OnTick, this), 1));
	request_timer_->Start();
	
	thread_.reset(new boost::thread(boost::bind(&TrackerRequestManager::ThreadFunc, this)));
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
void TrackerRequestManager::OnTick()
{
	boost::recursive_mutex::scoped_lock lock(processing_request_mutex_);

	FOREACH(element, processing_requests_)
	{
		element.second->OnTick();
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �̺߳���
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��11��02��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
void TrackerRequestManager::ThreadFunc()
{
	while (!stop_flag_)
	{
		TrackerRequestSP request;
		{
			boost::mutex::scoped_lock lock(request_mutex_);
			while (request_queue_.empty())
			{
				LOG(INFO) << "Waiting for incoming tracker request...";
				request_condition_.wait(lock);
			}

			request = request_queue_.front();
			request_queue_.pop();
		}

		request->Process();

		{
			boost::recursive_mutex::scoped_lock lock(processing_request_mutex_);
			processing_requests_.insert(
				RequestMap::value_type(request->tracker_address(), request));
		}
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������
 * ��  ��: [in] request ����
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��25��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
void TrackerRequestManager::AddRequest(const TrackerRequestSP& request)
{
	boost::mutex::scoped_lock lock(request_mutex_);

	request_queue_.push(request);

	request_condition_.notify_all();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������ɴ���
 * ��  ��: [in] request ����
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��10��25��
 * ���� teck_zhou
 * ���� ����
 ----------------------------------------------------------------------------*/
void TrackerRequestManager::OnRequestProcessed(const TrackerRequestSP& request)
{
	boost::recursive_mutex::scoped_lock lock(processing_request_mutex_);

	if (processing_requests_.empty()) return;

	// ��������
	RequestMapIter iter = processing_requests_.find(request->tracker_address());
	if (iter == processing_requests_.end())
	{
		LOG(ERROR) << "Can't find tracker request | " << request->tracker_address();
		return;
	}

	// ɾ������
	processing_requests_.erase(iter);
}

}
