/*#############################################################################
 * �ļ���   : mem_buffer_pool.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��07��
 * �ļ����� : BlockBufferPool����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_MEM_BUFFER_POOL
#define HEADER_MEM_BUFFER_POOL

#include "depend.hpp"
#include "connection.hpp"

namespace BroadCache
{

enum MsgBufferType
{
	MSG_BUF_COMMON,	// ��ͨ��Ϣ���棬һ�㳤�Ƚ϶�
	MSG_BUF_DATA    // ������Ӧ��Ϣ���棬һ�㳤�Ƚϳ�
};

enum DataBufferType
{
	DATA_BUF_BLOCK	// BLOCK��С
};

/******************************************************************************
 * ����: ����أ����ڹ���Session��ʹ�õ����ڴ��
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/12
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