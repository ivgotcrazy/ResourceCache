/*##############################################################################
 * 文件名   : protobuf_msg_encode.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.11
 * 文件描述 : 此文件声明了将protobuf消息编码，解码的几个全局函数 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_PROTOBUF_MSG_ENCODE
#define HEADER_PROTOBUF_MSG_ENCODE

#include <google/protobuf/message.h>
#include "bc_typedef.hpp"

namespace BroadCache
{

// 获取编码protobuf消息所需长度
msg_length_t GetProtobufMessageEncodeLength(const pb::Message& message);

// 编码protobuf消息
msg_length_t EncodeProtobufMessage(const pb::Message& message, char* buf, msg_length_t length);

// 解码protobuf消息
PbMessageSP DecodeProtobufMessage(const char* buf, msg_length_t length);

}  // namespace BroadCache

#endif  // HEADER_PROTOBUF_MSG_ENCODE
