/*##############################################################################
 * �ļ���   : message_recombiner.cpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.11
 * �ļ����� : ʵ����MessageRecombiner�� 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "message_recombiner.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: MessageRecombiner��Ĺ��캯��
 * ��  ��: [in] function ������Ϣ���ȵĺ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.11
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
MessageRecombiner::MessageRecombiner(const MessageLengthFunction& function)
    : message_length_function_(function), msg_length_(0), available_msg_pos_(msg_data_)
{
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ������Ϣ���ȵĺ���
 * ��  ��:
 * ����ֵ: ������Ϣ���ȵĺ���
 * ��  ��:
 *   ʱ�� 2013.11.11
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
MessageRecombiner::MessageLengthFunction MessageRecombiner::GetMessageLengthFunction() const
{
    return message_length_function_;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ü�����Ϣ���ȵĺ���
 * ��  ��: [in] function ������Ϣ���ȵĺ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.11
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void MessageRecombiner::SetMessageLengthFunction(const MessageLengthFunction& function)
{
    message_length_function_ = function;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ��Ϣ�б�
 * ��  ��: [in] data �˴�׷�ӵ���Ϣ����
 *         [in] length �˴�׷�ӵ���Ϣ���ݳ���
 * ����ֵ: ���������Ϣ�б�
 * ��  ��:
 *   ʱ�� 2013.11.11
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
MessageList MessageRecombiner::GetMessages(const char* data, msg_length_t length)
{
    AppendData(data, length);
    return GetMessages();
}

/*------------------------------------------------------------------------------
 * ��  ��: ����Ϣ������׷������
 * ��  ��: [in] data ����Ϣ������׷�ӵ���Ϣ����
 *         [in] length ����Ϣ������׷�ӵ����ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.11
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
void MessageRecombiner::AppendData(const char* data, msg_length_t length)
{
    BC_ASSERT(msg_length_ + length < kBufSize);

    memcpy(msg_data_ + msg_length_, data, length);
    msg_length_ += length;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ��Ϣ�б�
 * ��  ��:
 * ����ֵ: ��������Ϣ�б�
 * ��  ��:
 *   ʱ�� 2013.11.11
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
MessageList MessageRecombiner::GetMessages()
{
    msg_length_ -= (available_msg_pos_ - msg_data_);
    memmove(msg_data_, available_msg_pos_, msg_length_);  // ɾ���ϴ��Ѿ���ȡ����Ϣ����

    MessageList msgs;  // ���ص���Ϣ�б�
    available_msg_pos_ = msg_data_;  // ��ǰ��Ϣ����
    char* const kMsgDataEnd = msg_data_ + msg_length_;  // ��Ϣ���ݵ�βָ��

    msg_length_t calculated_msg_length = message_length_function_(available_msg_pos_,
        kMsgDataEnd - available_msg_pos_);

    while (available_msg_pos_ + calculated_msg_length <= kMsgDataEnd)
    {
        msgs.push_back(MessageEntry(available_msg_pos_, calculated_msg_length));  // �������б�������һ����Ϣ
        available_msg_pos_ += calculated_msg_length;

        
        calculated_msg_length = message_length_function_(available_msg_pos_,
            kMsgDataEnd - available_msg_pos_);
    } 

    return msgs;
}

}  // namespace BroadCache
