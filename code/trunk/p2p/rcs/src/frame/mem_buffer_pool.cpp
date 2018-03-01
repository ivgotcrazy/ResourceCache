/*#############################################################################
 * 文件名   : mem_buffer_pool.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月07日
 * 文件描述 : BlockBufferPool实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "mem_buffer_pool.hpp"
#include "bc_assert.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
MemBufferPool::MemBufferPool(uint64 common_msg_size, uint64 data_msg_size, 
	uint64 data_buf_size) 
	: common_msg_size_(common_msg_size)
	, data_msg_size_(data_msg_size)
	, data_buf_size_(data_buf_size)
    , common_msg_pool_(common_msg_size, MB_MAX_NUM)
	, data_msg_pool_(data_msg_size, MB_MAX_NUM)
	, data_buf_pool_(data_buf_size, MB_MAX_NUM)
{

}

/*-----------------------------------------------------------------------------
 * 描  述: 申请消息缓存
 * 参  数: [in] buf_type 缓存类型
 * 返回值: SendBuffer 调用这通过校验buf来判断申请是否成功
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
SendBuffer MemBufferPool::AllocMsgBuffer(MsgBufferType buf_type)
{
	boost::mutex::scoped_lock lock(mutex_);
	SendBuffer send_buf;

	switch (buf_type)
	{
	case MSG_BUF_COMMON:
		send_buf.buf = (char*)common_msg_pool_.ordered_malloc();
		send_buf.size = common_msg_size_;
		send_buf.free_func = boost::bind(&MemBufferPool::FreeMsgBuffer, this, MSG_BUF_COMMON, _1);
		break;

	case MSG_BUF_DATA:
		send_buf.buf = (char*)data_msg_pool_.ordered_malloc();
		send_buf.size = data_msg_size_;
		send_buf.free_func = boost::bind(&MemBufferPool::FreeMsgBuffer, this, MSG_BUF_DATA, _1);
		break;

	default:
		LOG(ERROR) << "Invalid message buffer type: " << buf_type;
		return SendBuffer();
	}	

	return send_buf;
}

/*-----------------------------------------------------------------------------
 * 描  述: 释放消息缓存
 * 参  数: [in] buf_type 缓存类型
 *         [in] buf 缓存地址
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MemBufferPool::FreeMsgBuffer(MsgBufferType buf_type, const char *buf)
{
	BC_ASSERT(buf);
	boost::mutex::scoped_lock lock(mutex_);

	switch (buf_type)
	{
	case MSG_BUF_COMMON:
		common_msg_pool_.ordered_free(const_cast<char*>(buf));
		break;

	case MSG_BUF_DATA:
		data_msg_pool_.ordered_free(const_cast<char*>(buf));
		break;

	default:
		LOG(ERROR) << "Invalid message buffer type: " << buf_type;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 申请数据缓存
 * 参  数: [in] buf_type 缓存类型
 * 返回值: 申请缓存地址
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
char* MemBufferPool::AllocDataBuffer(DataBufferType buf_type)
{
	boost::mutex::scoped_lock lock(mutex_);
	
	switch (buf_type)
	{
	case DATA_BUF_BLOCK:
		return (char *)data_buf_pool_.ordered_malloc();

	default:
		LOG(ERROR) << "Invalid message buffer type: " << buf_type;
		return 0;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 释放数据内存
 * 参  数: [in] buf_type 缓存类型
 *         [in] buf 缓存地址
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MemBufferPool::FreeDataBuffer(DataBufferType buf_type, const char *buf)
{
	BC_ASSERT(buf);
	boost::mutex::scoped_lock lock(mutex_);

	switch (buf_type)
	{
	case DATA_BUF_BLOCK:
		data_buf_pool_.ordered_free(const_cast<char*>(buf));
		break;

	default:
		LOG(ERROR) << "Invalid data buffer type: " << buf_type;
	}
}

}
