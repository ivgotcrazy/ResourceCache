/*#############################################################################
 * 文件名   : pkt_recombiner.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月19日
 * 文件描述 : PktRecombiner实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pkt_recombiner.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"
#include "msg_recognizer.hpp"

namespace BroadCache
{

static const uint32 kReservedBufSize = 16 * 1024;

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
PktRecombiner::PktRecombiner()
{
	pkt_buf_.reset(new PktBuf());
}

/*-----------------------------------------------------------------------------
 * 描  述: 注册消息识别器
 * 参  数: [in] recognizer 消息识别器
 *         [in] processor 消息处理函数
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::RegisterMsgRecognizer(MsgRecognizerSP recognizer, MsgProcessor processor)
{
	BC_ASSERT(recognizer);
	BC_ASSERT(processor);

	MsgParseEntry entry;
	entry.recognizer = recognizer;
	entry.processor  = processor;

	msg_parse_entries_.push_back(entry);
}

/*-----------------------------------------------------------------------------
 * 描  述: 附加数据
 * 参  数: [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::AppendData(const char* buf, uint32 len)
{
	BC_ASSERT(buf);

	if (len == 0) return;

	pkt_buf_->AppendData(buf, len);

	ProcessPkt();
}

/*-----------------------------------------------------------------------------
 * 描  述: 附加Udp数据
 * 参  数: [in] buf 数据
 *         [in] len 数据长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::AppendUdpData(const char* buf, uint32 len)
{
	BC_ASSERT(buf);

	if (len == 0) return;

	pkt_buf_->AppendUdpData(buf, len);

	ProcessUdpPkt();
}


/*-----------------------------------------------------------------------------
 * 描  述: 报文处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::ProcessPkt()
{
	bool processed;
	uint32 recognized_len;
	
	do
	{
		processed = false;
		for (MsgParseEntry& parse_entry : msg_parse_entries_)
		{
			if (parse_entry.recognizer->RecognizeMsg(pkt_buf_->GetBuf(), 
				pkt_buf_->GetBufSize(), &recognized_len))
			{
				parse_entry.processor(pkt_buf_->GetBuf(), recognized_len);

				pkt_buf_->RetriveData(recognized_len);

				processed = true;
				break;
			}
		}
	} while(processed && pkt_buf_->GetBufSize() != 0);
}

/*-----------------------------------------------------------------------------
 * 描  述: Udp报文处理
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::ProcessUdpPkt()
{
	uint32 recognized_len;
	
	for (MsgParseEntry& parse_entry : msg_parse_entries_)
	{
		if (parse_entry.recognizer->RecognizeMsg(pkt_buf_->GetBuf(), 
			pkt_buf_->GetBufSize(), &recognized_len))
		{
			parse_entry.processor(pkt_buf_->GetBuf(), recognized_len);

			break;
		}
	}
}

//==============================================================================

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
PktRecombiner::PktBuf::PktBuf() : read_index_(0), buf_size_(0)
{
	buf_.reserve(kReservedBufSize);
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: [in] buf 数据
 *         [in] len 长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::PktBuf::AppendData(const char* buf, uint32 len)
{
	uint32 final_size = read_index_ + buf_size_ + len;
	if (final_size > buf_.capacity()) // 空间不够
	{
		// 先往前腾挪一些空间
		memmove(&buf_[0], &buf_[read_index_], buf_size_);
		read_index_ = 0;

		// 如果还满足不了需求，则按照2倍扩充容量
		//TODO: 这个代码太挫了，后面看能否优化
		if (buf_.capacity() < buf_size_ + len)
		{
			std::vector<char> tmp(buf_size_);
			memcpy(&tmp[0], &buf_[0], buf_size_);

			buf_.reserve(final_size * 2);
			memcpy(&buf_[0], &tmp[0], buf_size_);
		}

		BC_ASSERT(buf_.capacity() >= buf_size_ + len);
	}

	memcpy(&buf_[read_index_ + buf_size_], buf, len);

	buf_size_ += len;

}

/*-----------------------------------------------------------------------------
 * 描  述: 添加Udp数据
 * 参  数: [in] buf 数据
 *         [in] len 长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::PktBuf::AppendUdpData(const char* buf, uint32 len)
{
	buf_.clear();

	if (len > buf_.capacity())
	{
		buf_.reserve(kReservedBufSize);
	}
	
	memcpy(&buf_[0], buf, len);

	buf_size_ = len;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取数据
 * 参  数: [in] len 长度
 * 返回值:
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktRecombiner::PktBuf::RetriveData(uint32 len)
{
	BC_ASSERT(len <= buf_size_);

	read_index_ += len;

	buf_size_ -= len;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取缓存数据首地址
 * 参  数: 
 * 返回值: 数据地址
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
const char* PktRecombiner::PktBuf::GetBuf()
{
	return &buf_[read_index_];
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取缓存数据长度
 * 参  数: 
 * 返回值: 数据长度
 * 修  改:
 *   时间 2013年11月19日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint32 PktRecombiner::PktBuf::GetBufSize()
{
	return buf_size_;
}

}