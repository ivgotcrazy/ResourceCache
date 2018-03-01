/*#############################################################################
 * 文件名   : ugs_config.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月14日
 * 文件描述 : UGS全局配置
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_UGS_UTIL
#define HEADER_UGS_UTIL

#include "bc_util.hpp"
#include "ugs_typedef.hpp"
#include "hash_stream.hpp"
#include "serialize.hpp"

namespace BroadCache
{

// 获取配置项
#define GET_UGS_CONFIG_INT(item, para) GET_CONFIG_INT(UgsConfigParser, item, para)
#define GET_UGS_CONFIG_STR(item, para) GET_CONFIG_STR(UgsConfigParser, item, para)
#define GET_UGS_CONFIG_BOOL(item, para) GET_CONFIG_BOOL(UgsConfigParser, item, para)

inline size_t hash_value(const ResourceId& resource)
{
    HashStream stream;
    stream & resource.protocol & resource.info_hash;

    return stream.value();
}

inline bool operator==(const ResourceId& lhs, const ResourceId& rhs)
{
    return (lhs.protocol == rhs.protocol) && (lhs.info_hash == rhs.info_hash);
}

inline bool operator!=(const ResourceId& lhs, const ResourceId& rhs)
{
    return !(lhs == rhs);
}

inline Serializer& serialize(Serializer& serializer, const ResourceId& resource)
{
    return serializer & resource.protocol & resource.info_hash;
}

inline Unserializer& unserialize(Unserializer& unserializer, ResourceId& resource)
{
    return unserializer & resource.protocol & resource.info_hash;
}

inline SizeHelper& serialize_size(SizeHelper& size_helper, const ResourceId& resource)
{
    return size_helper & resource.protocol & resource.info_hash;
}

}

#endif
