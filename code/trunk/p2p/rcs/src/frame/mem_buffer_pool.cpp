/*#############################################################################
 * �ļ���   : mem_buffer_pool.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��07��
 * �ļ����� : BlockBufferPoolʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "mem_buffer_pool.hpp"
#include "bc_assert.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ������Ϣ����
 * ��  ��: [in] buf_type ��������
 * ����ֵ: SendBuffer ������ͨ��У��buf���ж������Ƿ�ɹ�
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �ͷ���Ϣ����
 * ��  ��: [in] buf_type ��������
 *         [in] buf �����ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �������ݻ���
 * ��  ��: [in] buf_type ��������
 * ����ֵ: ���뻺���ַ
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �ͷ������ڴ�
 * ��  ��: [in] buf_type ��������
 *         [in] buf �����ַ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
