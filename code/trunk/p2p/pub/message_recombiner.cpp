/*##############################################################################
 * 文件名   : message_recombiner.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.11
 * 文件描述 : 实现了MessageRecombiner类 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#include "message_recombiner.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: MessageRecombiner类的构造函数
 * 参  数: [in] function 计算消息长度的函数
 * 返回值:
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
MessageRecombiner::MessageRecombiner(const MessageLengthFunction& function)
    : message_length_function_(function), msg_length_(0), available_msg_pos_(msg_data_)
{
}

/*------------------------------------------------------------------------------
 * 描  述: 获取计算消息长度的函数
 * 参  数:
 * 返回值: 计算消息长度的函数
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
MessageRecombiner::MessageLengthFunction MessageRecombiner::GetMessageLengthFunction() const
{
    return message_length_function_;
}

/*------------------------------------------------------------------------------
 * 描  述: 设置计算消息长度的函数
 * 参  数: [in] function 计算消息长度的函数
 * 返回值:
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void MessageRecombiner::SetMessageLengthFunction(const MessageLengthFunction& function)
{
    message_length_function_ = function;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取消息列表
 * 参  数: [in] data 此次追加的消息数据
 *         [in] length 此次追加的消息数据长度
 * 返回值: 解析后的消息列表
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
MessageList MessageRecombiner::GetMessages(const char* data, msg_length_t length)
{
    AppendData(data, length);
    return GetMessages();
}

/*------------------------------------------------------------------------------
 * 描  述: 向消息队列中追加数据
 * 参  数: [in] data 向消息队列中追加的消息数据
 *         [in] length 向消息队列中追加的数据长度
 * 返回值:
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
void MessageRecombiner::AppendData(const char* data, msg_length_t length)
{
    BC_ASSERT(msg_length_ + length < kBufSize);

    memcpy(msg_data_ + msg_length_, data, length);
    msg_length_ += length;
}

/*------------------------------------------------------------------------------
 * 描  述: 获取消息列表
 * 参  数:
 * 返回值: 解析后消息列表
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
MessageList MessageRecombiner::GetMessages()
{
    msg_length_ -= (available_msg_pos_ - msg_data_);
    memmove(msg_data_, available_msg_pos_, msg_length_);  // 删除上次已经获取的消息数据

    MessageList msgs;  // 返回的消息列表
    available_msg_pos_ = msg_data_;  // 当前消息数据
    char* const kMsgDataEnd = msg_data_ + msg_length_;  // 消息数据的尾指针

    msg_length_t calculated_msg_length = message_length_function_(available_msg_pos_,
        kMsgDataEnd - available_msg_pos_);

    while (available_msg_pos_ + calculated_msg_length <= kMsgDataEnd)
    {
        msgs.push_back(MessageEntry(available_msg_pos_, calculated_msg_length));  // 往返回列表中增加一个消息
        available_msg_pos_ += calculated_msg_length;

        
        calculated_msg_length = message_length_function_(available_msg_pos_,
            kMsgDataEnd - available_msg_pos_);
    } 

    return msgs;
}

}  // namespace BroadCache
