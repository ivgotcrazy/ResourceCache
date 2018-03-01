/*------------------------------------------------------------------------------
 * 文件名   : bt_net_serializer.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.10.07
 * 文件描述 : 专门针对bt消息实现一套序列化机制
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 -----------------------------------------------------------------------------*/
#ifndef HEADER_BT_NET_SERIALIZER
#define HEADER_BT_NET_SERIALIZER

#include "header_based_net_serializer.hpp"

namespace BroadCache
{

typedef HeaderBasedNetSerializer<uint32> BtNetSerializer;

/*------------------------------------------------------------------------------
 *描  述: 针对bt协议，对HeaderBasedNetUnserializer进行包装
 *作  者: rosan
 *时  间: 2013.10.07
 -----------------------------------------------------------------------------*/
class BtNetUnserializer : public HeaderBasedNetUnserializer<uint32>
{
public:
    enum { BT_MSG_TYPE_LENGTH = 1 }; //bt消息类型所占的字节数

    // NetUnserializer跳过消息头部和消息类型
    explicit BtNetUnserializer(const char* buf) 
        : HeaderBasedNetUnserializer<uint32>(buf) 
    {
        (*this)->advance(BT_MSG_TYPE_LENGTH);
    }
};

}

#endif
