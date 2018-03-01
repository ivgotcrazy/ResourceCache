/*##############################################################################
 * 文件名   : message_recombiner.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.11
 * 文件描述 :  
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_MESSAGE_RECOMBINER
#define HEADER_MESSAGE_RECOMBINER

#include "bc_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 *描  述: 此类主要实现网络消息的分割和合并功能
 *作  者: rosan
 *时  间: 2013.11.11
 ******************************************************************************/
class MessageRecombiner
{
public:
    // 用于计算消息长度的函数
    typedef boost::function<msg_length_t(const char*, msg_length_t)> MessageLengthFunction;

public:
    explicit MessageRecombiner(const MessageLengthFunction& function = MessageLengthFunction());
    
    // 将消息合并，分割
    MessageList GetMessages(const char* data, msg_length_t length);
    
    // 获取，设置用于计算消息长度的函数
    MessageLengthFunction GetMessageLengthFunction() const;
    void SetMessageLengthFunction(const MessageLengthFunction& function);

private:
    // 追加消息数据
    void AppendData(const char* data, msg_length_t length);

    // 获取现有的消息列表
    MessageList GetMessages();

private:
    static const msg_length_t kBufSize = 65536;

    MessageLengthFunction message_length_function_;  // 计算消息长度的函数
    msg_length_t msg_length_;  // 消息数据长度
    char* available_msg_pos_;  // 可以获取的第一个消息的位置
    char msg_data_[kBufSize];  // 消息数据缓冲区
};

}  // namespace BroadCache

#endif  // HEADER_MESSAGE_RECOMBINER
