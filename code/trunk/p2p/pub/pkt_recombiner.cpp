/*#############################################################################
 * �ļ���   : pkt_recombiner.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��19��
 * �ļ����� : PktRecombinerʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pkt_recombiner.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"
#include "msg_recognizer.hpp"

namespace BroadCache
{

static const uint32 kReservedBufSize = 16 * 1024;

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PktRecombiner::PktRecombiner()
{
	pkt_buf_.reset(new PktBuf());
}

/*-----------------------------------------------------------------------------
 * ��  ��: ע����Ϣʶ����
 * ��  ��: [in] recognizer ��Ϣʶ����
 *         [in] processor ��Ϣ������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��������
 * ��  ��: [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PktRecombiner::AppendData(const char* buf, uint32 len)
{
	BC_ASSERT(buf);

	if (len == 0) return;

	pkt_buf_->AppendData(buf, len);

	ProcessPkt();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Udp����
 * ��  ��: [in] buf ����
 *         [in] len ���ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PktRecombiner::AppendUdpData(const char* buf, uint32 len)
{
	BC_ASSERT(buf);

	if (len == 0) return;

	pkt_buf_->AppendUdpData(buf, len);

	ProcessUdpPkt();
}


/*-----------------------------------------------------------------------------
 * ��  ��: ���Ĵ���
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: Udp���Ĵ���
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PktRecombiner::PktBuf::PktBuf() : read_index_(0), buf_size_(0)
{
	buf_.reserve(kReservedBufSize);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: [in] buf ����
 *         [in] len ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PktRecombiner::PktBuf::AppendData(const char* buf, uint32 len)
{
	uint32 final_size = read_index_ + buf_size_ + len;
	if (final_size > buf_.capacity()) // �ռ䲻��
	{
		// ����ǰ��ŲһЩ�ռ�
		memmove(&buf_[0], &buf_[read_index_], buf_size_);
		read_index_ = 0;

		// ��������㲻����������2����������
		//TODO: �������̫���ˣ����濴�ܷ��Ż�
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
 * ��  ��: ���Udp����
 * ��  ��: [in] buf ����
 *         [in] len ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��ȡ����
 * ��  ��: [in] len ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PktRecombiner::PktBuf::RetriveData(uint32 len)
{
	BC_ASSERT(len <= buf_size_);

	read_index_ += len;

	buf_size_ -= len;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ���������׵�ַ
 * ��  ��: 
 * ����ֵ: ���ݵ�ַ
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
const char* PktRecombiner::PktBuf::GetBuf()
{
	return &buf_[read_index_];
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�������ݳ���
 * ��  ��: 
 * ����ֵ: ���ݳ���
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint32 PktRecombiner::PktBuf::GetBufSize()
{
	return buf_size_;
}

}