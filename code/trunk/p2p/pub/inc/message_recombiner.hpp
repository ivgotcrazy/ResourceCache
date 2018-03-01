/*##############################################################################
 * �ļ���   : message_recombiner.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.11
 * �ļ����� :  
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_MESSAGE_RECOMBINER
#define HEADER_MESSAGE_RECOMBINER

#include "bc_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 *��  ��: ������Ҫʵ��������Ϣ�ķָ�ͺϲ�����
 *��  ��: rosan
 *ʱ  ��: 2013.11.11
 ******************************************************************************/
class MessageRecombiner
{
public:
    // ���ڼ�����Ϣ���ȵĺ���
    typedef boost::function<msg_length_t(const char*, msg_length_t)> MessageLengthFunction;

public:
    explicit MessageRecombiner(const MessageLengthFunction& function = MessageLengthFunction());
    
    // ����Ϣ�ϲ����ָ�
    MessageList GetMessages(const char* data, msg_length_t length);
    
    // ��ȡ���������ڼ�����Ϣ���ȵĺ���
    MessageLengthFunction GetMessageLengthFunction() const;
    void SetMessageLengthFunction(const MessageLengthFunction& function);

private:
    // ׷����Ϣ����
    void AppendData(const char* data, msg_length_t length);

    // ��ȡ���е���Ϣ�б�
    MessageList GetMessages();

private:
    static const msg_length_t kBufSize = 65536;

    MessageLengthFunction message_length_function_;  // ������Ϣ���ȵĺ���
    msg_length_t msg_length_;  // ��Ϣ���ݳ���
    char* available_msg_pos_;  // ���Ի�ȡ�ĵ�һ����Ϣ��λ��
    char msg_data_[kBufSize];  // ��Ϣ���ݻ�����
};

}  // namespace BroadCache

#endif  // HEADER_MESSAGE_RECOMBINER
