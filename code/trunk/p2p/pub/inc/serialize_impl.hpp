/*------------------------------------------------------------------------------
 * 文件名   : serialize_impl.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.08.28
 * 文件描述 : 实现对象的序列化
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_SERIALIZE_IMPL
#define HEADER_SERIALIZE_IMPL

#include <cstring>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 序列化一个对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 * 返回值: 序列化一个对象后的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Serializer& operator&(Serializer& serializer, const T& t)
{
    return Serialize(serializer, t);
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 * 返回值: 反序列化一个对象后的反序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& operator&(Unserializer& unserializer, T& t)
{
    return Unserialize(unserializer, t);
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个对象所需的缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t T类型的对象
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& operator&(SizeHelper& size_helper, const T& t)
{
    return SerializeSize(size_helper, t);
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化一个对象
 * 参  数: [in][out] serializer 序列化器
           [in] t 被序列化的对象
 * 返回值: 参数中的序列化器
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t)
{
    return Serialize(serializer, t, TypeTag<static_cast<SerializeTag>(TagTrait<T>::value)>());
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化pod类型的对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_POD> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_POD>)
{
    std::memcpy(serializer.value(), &t, sizeof(T));
    serializer.advance(sizeof(T));

    return serializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化std::basic_string类型的对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_STRING> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_STRING>)
{
    serializer & t.size();
    std::memcpy(serializer.value(), t.c_str(), t.size() * sizeof(typename T::value_type));
    serializer.advance(t.size() * sizeof(typename T::value_type));

    return serializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化C风格的字符串
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_CSTRING> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_CSTRING>)
{
    std::strcpy(serializer.value(), t);
    serializer.advance(std::strlen(t) + 1);

    return serializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化顺序容器的对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_SEQ_CONTAINER> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_SEQ_CONTAINER>)
{
    serializer & t.size();
    for (const typename T::value_type& elem : t)
    {
        serializer & elem;
    }

    return serializer; 
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化顺序容器的对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_PAIR> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_PAIR>)
{
    return serializer & t.first & t.second;
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化顺序容器的对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_SET> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_SET>)
{
    serializer & t.size();
    for (const typename T::value_type& elem : t)
    {
        serializer & elem;
    }

    return serializer; 
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化顺序容器的对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_MAP> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_MAP>)
{
    serializer & t.size();
    for (const typename T::value_type& elem : t)
    {
        serializer & elem;
    }

    return serializer; 
}

/*------------------------------------------------------------------------------
 * 描  述: 序列化其它类型的对象
 * 参  数: [in][out] serializer 序列化器
 *         [in] t 被序列化的对象
 *         [in] TypeTag<TAG_HAS_IMPL> dummy参数，占位符
 * 返回值: 输入的序列化器
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T> 
inline Serializer& Serialize(Serializer& serializer, const T& t, TypeTag<TAG_HAS_IMPL>)
{
    return serialize(serializer, t);
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t)
{
    return Unserialize(unserializer, t, TypeTag<static_cast<SerializeTag>(TagTrait<T>::value)>());
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个POD类型对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_POD> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_POD>)
{
    std::memcpy(&t, unserializer.value(), sizeof(T));
    unserializer.advance(sizeof(T));

    return unserializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个std::basic_string类型对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_STRING> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_STRING>)
{
    typename T::size_type size = 0;
    unserializer & size;
    t.resize(size);
    t.assign(reinterpret_cast<const typename T::value_type*>(unserializer.value()), size);
    unserializer.advance(size * sizeof(typename T::value_type)); 

    return unserializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个C风格的字符串
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_CSTRING> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_CSTRING>)
{
    std::strcpy(t, unserializer.value());
    unserializer.advance(std::strlen(unserializer.value()) + 1);

    return unserializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个序列容器对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_SEQ_CONTAINTER> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_SEQ_CONTAINER>)
{
    typename T::size_type size = 0;
    unserializer & size;  // 设置容器大小
    t.resize(size);
    for (typename T::value_type& elem : t)  // 序列化容器中的每一个对象
    {
        unserializer & elem;
    }
    
    return unserializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个序列容器对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_PAIR> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_PAIR>)
{
    return unserializer & const_cast<typename boost::remove_const<decltype(t.first)>::type&>(t.first) & t.second;
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个序列容器对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_SET> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_SET>)
{
    typename T::size_type size = 0;
    unserializer & size;

    while (size > 0)
    {
        typename T::value_type elem;
        unserializer & elem;
        t.insert(elem);

        --size;
    }

    return unserializer;
}

/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个序列容器对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_MAP> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_MAP>)
{
    typename T::size_type size = 0;
    unserializer & size;

    while (size > 0)
    {
        typename T::value_type elem;
        unserializer & elem;
        t.insert(elem);

        --size;
    }
 
    return unserializer;
}


/*------------------------------------------------------------------------------
 * 描  述: 反序列化一个其它对象
 * 参  数: [in][out] unserializer 反序列化器
 *         [out] t 被反序列化的对象
 *         [in] TypeTag<TAG_HAS_IMPL> dummy参数，占位符
 * 返回值: 参数中的反序列化器 
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline Unserializer& Unserialize(Unserializer& unserializer, T& t, TypeTag<TAG_HAS_IMPL>)
{
    return unserialize(unserializer, t);
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t)
{
    return SerializeSize(size_helper, t, TypeTag<static_cast<SerializeTag>(TagTrait<T>::value)>());
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个pod类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_POD> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& /*t*/, TypeTag<TAG_POD>)
{
    size_helper.advance(sizeof(T));
    return size_helper; 
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个std::basic_string类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_STRING> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_STRING>)
{
    size_helper.advance(sizeof(typename T::size_type) + t.size() * sizeof(typename T::value_type));
    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个C风格的字符串所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_CSTRING> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_CSTRING>)
{
    size_helper.advance(std::strlen(t) + 1);
    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_SEQ_CONTAINTER> dummy参数，占位符
 *         [in] TypeTag<tag> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T, SerializeTag tag>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SEQ_CONTAINER>, TypeTag<tag>)
{
    size_helper & t.size();

    for (const typename T::value_type& elem : t)
    {
        size_helper & elem;
    }

    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_SEQ_CONTAINTER> dummy参数，占位符
 *         [in] TypeTag<TAG_POD> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SEQ_CONTAINER>, TypeTag<TAG_POD>)
{
    size_helper & t.size();
    size_helper.advance(t.size() * sizeof(T::value_type));

    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_SEQ_CONTAINTER> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SEQ_CONTAINER> tag)
{
    return SerializeSize(size_helper, t, tag, 
        TypeTag<static_cast<SerializeTag>(TagTrait<typename T::value_type>::value)>());
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_PAIR> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_PAIR>)
{
    return size_helper & t.first & t.second;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_SET> dummy参数，占位符
 *         [in] TypeTag<tag> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T, SerializeTag tag>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SET>, TypeTag<tag>)
{
    size_helper & t.size();

    for (const typename T::value_type& elem : t)
    {
        size_helper & elem;
    }

    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_SET> dummy参数，占位符
 *         [in] TypeTag<TAG_POD> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SET>, TypeTag<TAG_POD>)
{
    size_helper & t.size();
    size_helper.advance(t.size() * sizeof(T::value_type));

    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] tag dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_SET> tag)
{
    return SerializeSize(size_helper, t, tag, 
        TypeTag<static_cast<SerializeTag>(TagTrait<typename T::value_type>::value)>());
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_MAP> dummy参数，占位符
 *         [in] boost::integral_const<bool, true> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_MAP>,
                                 boost::integral_constant<bool, false>)
{
    size_helper & t.size();

    for (const typename T::value_type& elem : t)
    {
        size_helper & elem;
    }

    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_MAP> dummy参数，占位符
 *         [in] boost::integral_const<bool, true> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_MAP>,
                                 boost::integral_constant<bool, true>)
{
    size_helper & t.size();
    size_helper.advance(t.size() * (sizeof(typename T::key_type) + sizeof(typename T::mapped_type)));

    return size_helper;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个序列容器类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] tag dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_MAP> tag)
{
    return SerializeSize(size_helper, t, tag, boost::integral_constant<bool,
        boost::is_pod<typename T::key_type>::value && boost::is_pod<typename T::mapped_type>::value>());
}

/*------------------------------------------------------------------------------
 * 描  述: 计算序列化一个其它类型对象所需缓冲区大小
 * 参  数: [in][out] size_helper 用于计算序列化一个对象所需缓冲区大小
 *         [in] t 被计算的对象
 *         [in] TypeTag<TAG_HAS_IMPL> dummy参数，占位符
 * 返回值: 参数中的SizeHelper对象
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class T>
inline SizeHelper& SerializeSize(SizeHelper& size_helper, const T& t, TypeTag<TAG_HAS_IMPL>)
{
    return serialize_size(size_helper, t);
}

}

#endif  // HEADER_SERIALIZE_IMPL
