/*##############################################################################
 * 文件名   : header_based_net_serializer.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.11
 * 文件描述 : 此文件主要包含两个模板类HeaderBasedNetSerializer和HeaderBasedNetUnserializer 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_HEADER_BASED_NET_SERIALIZER
#define HEADER_HEADER_BASED_NET_SERIALIZER

#include "bc_assert.hpp"
#include "net_byte_order.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 获取基于消息头部（保存整个消息的长度）的消息总长度（包括消息头部）
 * 参  数: [in] buf 消息数据
 *         [in] length 消息长度
 * 返回值: 此消息的总长度
 * 修  改:
 *   时间 2013.11.12
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<typename HeaderDataType>
inline msg_length_t GetHeaderBasedMsgLength(const char* buf, msg_length_t length)
{
    BC_ASSERT(sizeof(HeaderDataType) <= length);

    return msg_length_t(NetToHost<HeaderDataType>(buf) + sizeof(HeaderDataType));
}

/*******************************************************************************
 *描  述: 此类主要实现在消息头部自动填充消息长度的功能
          模板参数HeaderDataType指定了消息头部的消息长度的数据类型
 *作  者: rosan
 *时  间: 2013.11.11
 ******************************************************************************/
template<typename HeaderDataType>
class HeaderBasedNetSerializer
{
public:
    typedef HeaderDataType HeaderType;  // 消息头部的消息长度的数据类型

public:
    explicit HeaderBasedNetSerializer(char* buf)
        : begin_(buf), net_serializer_(buf + sizeof(HeaderType))
    {
    }

    // 等整个消息的消息体部分填充完毕，再填充消息头部长度
    HeaderType Finish()
    {
        HeaderType length(net_serializer_.value() - begin_);  // 获取消息体长度
        
        // 填充消息头部
        net_serializer_.set_value(begin_);
        net_serializer_ & HeaderType(length - sizeof(HeaderType));

        return length; 
    }

    // 重载类型转换操作符
    operator NetSerializer&()
    {
        return net_serializer_;
    }

    // 重载指针操作符
    NetSerializer* operator->()
    {
        return &net_serializer_;
    }

private:
    char* begin_;  // 消息头部位置
    NetSerializer net_serializer_;  // 网络序列化对象
};

/*******************************************************************************
 *描  述: 此类主要实现自动跳过消息头部的功能
          模板参数HeaderDataType指定了消息头部的消息长度的数据类型
 *作  者: rosan
 *时  间: 2013.11.11
 ******************************************************************************/
template<typename HeaderDataType>
class HeaderBasedNetUnserializer
{
public:
    typedef HeaderDataType HeaderType;  // 消息头部的消息长度的数据类型

public:
    //跳过消息头部（长度）
    explicit HeaderBasedNetUnserializer(const char* buf)
        : begin_(buf), net_unserializer_(buf + sizeof(HeaderType))
    {
    }

    // 获取消息的总长度（包括消息头部）
    HeaderType GetLength() const
    {
        return NetToHost<HeaderType>(begin_) + sizeof(HeaderType);
    }

    // 重载类型转换操作符
    operator NetUnserializer&()
    {
        return net_unserializer_;
    }

    // 重载指针操作符
    NetUnserializer* operator->()
    {
        return &net_unserializer_;
    }

private:
    const char* begin_;  // 消息头部位置
    NetUnserializer net_unserializer_;  // 网络反序列化对象
}; 

}  // namespace BroadCache

#endif  // HEADER_HEADER_BASED_NET_SERIALIZER
