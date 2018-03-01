#include "protobuf_msg_encode.hpp"
#include <google/protobuf/descriptor.h>
#include "bc_util.hpp"
#include "header_based_net_serializer.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 根据类型名创建一个protobuf Message对象
 * 参  数: [in] type_name 待创建的protobuf Message对象类型名
 * 返回值: 创建的protobuf Message对象
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline static pb::Message* CreateMessage(const std::string& type_name)
{
    return pb::MessageFactory::generated_factory()->GetPrototype(
            pb::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name))->New();
}

// struct ProtobufTransportFormat
// {
//     int32 message_length;
//     int32 type_name_length;
//     char type_name[type_name_length];  // ends up with '\0'
//     char protobuf_data[message_length - sizeof(type_name_length) - type_name_length - sizeof(checksum)];
//     int32 checksum;
// };

//pb 是protobuf的简写
typedef int32 pb_length_t;
typedef int32 pb_checksum_t;

/*------------------------------------------------------------------------------
 * 描  述: 获取编码protobuf消息所需长度
 * 参  数: [in] message 待编码的protobuf消息对象
 * 返回值: protobuf消息编码所需长度
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
msg_length_t GetProtobufMessageEncodeLength(const pb::Message& message)
{
    std::string type_name = message.GetTypeName();
    
    return msg_length_t(sizeof(pb_length_t)
        + sizeof(pb_length_t)
        + type_name.size() + 1
        + message.ByteSize()
        + sizeof(pb_checksum_t));
}

/*------------------------------------------------------------------------------
 * 描  述: 按ProtobufTransportFormat格式编码protobuf Message对象
 * 参  数: [in] message 待编码的protobuf Message对象
 *         [out] buf 编码的缓冲区
 *         [in] length 编码的缓冲区大小
 * 返回值: protobuf Message实际编码占用的缓冲区大小
 *         如果为0则表示编码失败
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
msg_length_t EncodeProtobufMessage(const pb::Message& message, 
                                   char* buf, msg_length_t length)
{
    if (length < GetProtobufMessageEncodeLength(message))
    {
        return 0;
    }

    std::string type_name = message.GetTypeName();  // 消息类型
    auto name_size = type_name.size() + 1;  // 消息类型长度，包括终止符'\0'

    HeaderBasedNetSerializer<pb_length_t> serializer(buf);

    serializer & pb_length_t(name_size);  // 写入消息类型长度

    // 写入消息类型
    memcpy(serializer->value(), type_name.c_str(), name_size);
    serializer->advance(name_size);

    // 写入protobuf数据
    auto msg_byte_size = message.ByteSize();
    message.SerializeToArray(serializer->value(), msg_byte_size);
    serializer->advance(msg_byte_size);

    // 写入crc checksum（校验type_name_length，type_name，protobuf_data）
    char* checksum_begin = buf + sizeof(pb_length_t);
    serializer & pb_checksum_t(GetCrc32Checksum(checksum_begin, 
        serializer->value() - checksum_begin));

    return msg_length_t(serializer.Finish());  // 在头部写入消息长度
}

/*------------------------------------------------------------------------------
 * 描  述: 按ProtobufTransportFormat格式解码protobuf Message对象
 * 参  数: [in] buf 待解码的缓冲区
 *         [in] length 待解码的缓冲区大小
 * 返回值: 解码后创建的protobuf Message对象
 * 修  改:
 *   时间 2013.11.11
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
PbMessageSP DecodeProtobufMessage(const char* buf, msg_length_t length)
{
    PbMessageSP message;

    HeaderBasedNetUnserializer<pb_length_t> unserializer(buf);
    BC_ASSERT(unserializer.GetLength() == pb_length_t(length));

    // 读取消息类型的长度
    pb_length_t type_name_length = 0;
    unserializer & type_name_length;

    // 读取消息类型
    std::string type_name(unserializer->value());
    unserializer->advance(type_name_length);

    message.reset(CreateMessage(type_name));
    if (!message)  // 消息类型不合法
    {
        return PbMessageSP();
    }

    // 读取protobuf消息数据
    const char* protobuf_data = unserializer->value();
    msg_length_t data_length = length - sizeof(pb_length_t) - sizeof(pb_length_t)
        - type_name_length - sizeof(pb_checksum_t);
    unserializer->advance(data_length);

    // 读取crc32校验和
    pb_checksum_t check_sum = 0;
    unserializer & check_sum;

    if (uint32(check_sum) != GetCrc32Checksum(buf + sizeof(pb_length_t),
        length - sizeof(pb_length_t) - sizeof(pb_checksum_t)))  // 校验和错误
    {
        return PbMessageSP();
    }

    if (!message->ParseFromArray(protobuf_data, data_length))  // 解析消息数据失败
    {
        return PbMessageSP();
    }
    
    return message; 
}

}  // namespace BroadCache
