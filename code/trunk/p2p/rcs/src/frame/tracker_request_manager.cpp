/*#############################################################################
 * 文件名   : tracker_request_manager.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年10月30日
 * 文件描述 : TrackerRequestManager实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "tracker_request_manager.hpp"
#include "tracker_request.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 获取单例
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年10月25日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
TrackerRequestManager& TrackerRequestManager::GetInstance()
{
	static TrackerRequestManager manager;
	return manager;
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月03日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
TrackerRequestManager::TrackerRequestManager() : stop_flag_(false)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月03日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
TrackerRequestManager::~TrackerRequestManager()
{
	stop_flag_ = true;

	thread_->join();
}

/*-----------------------------------------------------------------------------
 * 描  述: 创建线程
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月02日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequestManager::Init(io_service& ios)
{
	request_timer_.reset(new Timer(ios, boost::bind(&TrackerRequestManager::OnTick, this), 1));
	request_timer_->Start();
	
	thread_.reset(new boost::thread(boost::bind(&TrackerRequestManager::ThreadFunc, this)));
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
void TrackerRequestManager::OnTick()
{
	boost::recursive_mutex::scoped_lock lock(processing_request_mutex_);

	FOREACH(element, processing_requests_)
	{
		element.second->OnTick();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 线程函数
 * 参  数: 
 * 返回值:
 * 修  改:
 * 时间 2013年11月02日
 * 作者 teck_zhou
 * 描述 创建
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
 * 描  述: 添加请求
 * 参  数: [in] request 请求
 * 返回值:
 * 修  改:
 * 时间 2013年10月25日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequestManager::AddRequest(const TrackerRequestSP& request)
{
	boost::mutex::scoped_lock lock(request_mutex_);

	request_queue_.push(request);

	request_condition_.notify_all();
}

/*-----------------------------------------------------------------------------
 * 描  述: 请求完成处理
 * 参  数: [in] request 请求
 * 返回值:
 * 修  改:
 * 时间 2013年10月25日
 * 作者 teck_zhou
 * 描述 创建
 ----------------------------------------------------------------------------*/
void TrackerRequestManager::OnRequestProcessed(const TrackerRequestSP& request)
{
	boost::recursive_mutex::scoped_lock lock(processing_request_mutex_);

	if (processing_requests_.empty()) return;

	// 查找请求
	RequestMapIter iter = processing_requests_.find(request->tracker_address());
	if (iter == processing_requests_.end())
	{
		LOG(ERROR) << "Can't find tracker request | " << request->tracker_address();
		return;
	}

	// 删除请求
	processing_requests_.erase(iter);
}

}
