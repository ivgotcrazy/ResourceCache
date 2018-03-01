/*#############################################################################
 * 文件名   : mem_buffer_pool.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月07日
 * 文件描述 : BlockBufferPool声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_MEM_BUFFER_POOL
#define HEADER_MEM_BUFFER_POOL

#include "depend.hpp"
#include "connection.hpp"

namespace BroadCache
{

enum MsgBufferType
{
	MSG_BUF_COMMON,	// 普通消息缓存，一般长度较短
	MSG_BUF_DATA    // 数据响应消息缓存，一般长度较长
};

enum DataBufferType
{
	DATA_BUF_BLOCK	// BLOCK大小
};

/******************************************************************************
 * 描述: 缓存池，用于管理Session中使用到的内存池
 * 作者：teck_zhou
 * 时间：2013/10/12
 *****************************************************************************/
class MemBufferPool : public boost::noncopyable
{
public:
	MemBufferPool(uint64 common_msg_size, 
		          uint64 data_msg_size, 
				  uint64 data_buf_size);

	SendBuffer AllocMsgBuffer(MsgBufferType buf_type);
	void FreeMsgBuffer(MsgBufferType buf_type, const char* buf);

	char* AllocDataBuffer(DataBufferType buf_type);
	void FreeDataBuffer(DataBufferType buf_type, const char* buf);

private:
	boost::mutex mutex_;

	uint64 common_msg_size_;
	uint64 data_msg_size_;
	uint64 data_buf_size_;

	boost::pool<> common_msg_pool_;
	boost::pool<> data_msg_pool_;
	boost::pool<> data_buf_pool_;
};

}

#endif