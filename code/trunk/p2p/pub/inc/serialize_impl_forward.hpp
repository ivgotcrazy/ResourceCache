/*------------------------------------------------------------------------------
 * 文件名   : serialize_impl_forward.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.08.28
 * 文件描述 : 序列化实现的前置声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_SERIALIZE_IMPL_FORWARD
#define HEADER_SERIALIZE_IMPL_FORWARD

#include "serialize_tag.hpp"
#include "single_value_model.hpp"

namespace BroadCache
{

static const size_t kSerializeClass = 0;

typedef SingleValueModel<char*, kSerializeClass> Serializer;  // 序列化器
typedef SingleValueModel<const char*, kSerializeClass> Unserializer;  // 反序列化器
typedef SingleValueModel<size_t, kSerializeClass> SizeHelper;  // 用于获取一个对象序列化所需缓冲区大小

template<class T>
inline Serializer& operator&(Serializer& serializer, const T& t);

template<class T>
inline Unserializer& operator&(Unserializer& unserializer, T& t);

template<class T>
inline SizeHelper& operator&(SizeHelper& size_helper, const T& t);

template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_POD>);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_STRING>);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_CSTRING>);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_SEQ_CONTAINER>);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_PAIR>);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_SET>);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_MAP>);
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_HAS_IMPL>);

template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_POD>);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_STRING>);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_CSTRING>);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_SEQ_CONTAINER>);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_PAIR>);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_SET>);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_MAP>);
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_HAS_IMPL>);

template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_POD>);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_STRING>);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_CSTRING>);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SEQ_CONTAINER>);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_PAIR>);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SET>);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_MAP>);
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_HAS_IMPL>);

}

#endif  // HEADER_SERIALIZE_IMPL_FORWARD
