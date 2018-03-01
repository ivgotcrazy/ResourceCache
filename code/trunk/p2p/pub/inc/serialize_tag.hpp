/*------------------------------------------------------------------------------
 * 文件名   : serialize_tag.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.08.28
 * 文件描述 : 此文件主要实现对类型进行标签绑定
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_SERIALIZE_TAG
#define HEADER_SERIALIZE_TAG

#include <deque>
#include <list>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/type_traits.hpp>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 此宏实现将一种类型绑定为POD标签
 * 参  数: POD类型
 * 返回值:
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
#define POD_BINDER(pod_type)\
template<>\
struct TagTrait<pod_type>\
{\
    enum { value = TAG_POD };\
};\

/*------------------------------------------------------------------------------
 * 描  述: 此宏主要实现将容器类型绑定为容器标签
 * 参  数: 容器类型
 * 返回值:
 * 修  改:
 *   时间 2013.08.28
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
#define SEQ_CONTAINER_BINDER(container_type)\
template<class _Ty,\
    class _Ax>\
struct TagTrait<container_type<_Ty, _Ax> >\
{\
    enum { value = TAG_SEQ_CONTAINER };\
};\

/*------------------------------------------------------------------------------
 *描  述: 类型标签
 *作  者: rosan
 *时  间: 2013.08.28
 -----------------------------------------------------------------------------*/ 
enum SerializeTag
{
    TAG_NULL,
    TAG_POD,   // POD类型
    TAG_STRING,  // 标准库字符串类型
    TAG_CSTRING,  // C形式的字符串
    TAG_SEQ_CONTAINER,  // 顺序容器
    TAG_PAIR,  // 键值对 pair
    TAG_SET,  // 集合set, multiset, unordered_set, unordered_multiset
    TAG_MAP,  // 键值对容器, map, multimap, unordered_map, unordered_multimap
    TAG_HAS_IMPL,  // 此类型已经有序列化实现函数
};

/*------------------------------------------------------------------------------
 *描  述: 为每一种数据类型对应的枚举(SerializeTag)
          绑定一种标签(TypeTag<SerializeTag>)
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<SerializeTag tag>
struct TypeTag
{
};

/*------------------------------------------------------------------------------
 *描  述: 定义其它类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<typename T>
struct TagTrait
{
    enum { value = (boost::is_pod<T>::value ? TAG_POD : TAG_HAS_IMPL) };
};

POD_BINDER(bool)
POD_BINDER(char)
POD_BINDER(unsigned char)
POD_BINDER(short)
POD_BINDER(unsigned short)
POD_BINDER(int)
POD_BINDER(unsigned int)
POD_BINDER(float)
POD_BINDER(long)
POD_BINDER(unsigned long)
POD_BINDER(double)

/*------------------------------------------------------------------------------
 *描  述: 定义std::basic_string类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class _Elem, 
    class _Traits, 
    class _Ax>
struct TagTrait<std::basic_string<_Elem, _Traits, _Ax> >
{
    enum { value = TAG_STRING };
};

SEQ_CONTAINER_BINDER(std::vector)
SEQ_CONTAINER_BINDER(std::deque)
SEQ_CONTAINER_BINDER(std::list)

/*------------------------------------------------------------------------------
 *描  述: 定义char*类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<>
struct TagTrait<char*>
{
    enum { value = TAG_CSTRING };
};

/*------------------------------------------------------------------------------
 *描  述: 定义const char*类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<>
struct TagTrait<const char*>
{
    enum { value = TAG_CSTRING };
};

/*------------------------------------------------------------------------------
 *描  述: 定义unordered_set类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class T,
    class H,
    class P,
    class A>
struct TagTrait<boost::unordered_set<T, H, P, A> >
{
    enum { value = TAG_SET };
};

/*------------------------------------------------------------------------------
 *描  述: 定义unordered_multiset类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class T,
    class H,
    class P,
    class A>
struct TagTrait<boost::unordered_multiset<T, H, P, A> >
{
    enum { value = TAG_SET };
};

/*------------------------------------------------------------------------------
 *描  述: 定义set类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class _Kty,
    class _Pr,
    class _Alloc>
struct TagTrait<std::set<_Kty, _Pr, _Alloc> >
{
    enum { value = TAG_SET };
};

/*------------------------------------------------------------------------------
 *描  述: 定义multiset类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class _Kty,
    class _Pr,
    class _Alloc>
struct TagTrait<std::multiset<_Kty, _Pr, _Alloc> >
{
    enum { value = TAG_SET };
};

/*------------------------------------------------------------------------------
 *描  述: 定义unordered_map类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class K,
    class T,
    class H,
    class P,
    class A>
struct TagTrait<boost::unordered_map<K, T, H, P, A> >
{
    enum { value = TAG_MAP };
};

/*------------------------------------------------------------------------------
 *描  述: 定义unordered_multimap类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class K,
    class T,
    class H,
    class P,
    class A>
struct TagTrait<boost::unordered_multimap<K, T, H, P, A> >
{
    enum { value = TAG_MAP };
};

/*------------------------------------------------------------------------------
 *描  述: 定义map类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class _Kty,
    class _Ty,
    class _Pr,
    class _Alloc>
struct TagTrait<std::map<_Kty, _Ty, _Pr, _Alloc> >
{
    enum { value = TAG_MAP };
};

/*------------------------------------------------------------------------------
 *描  述: 定义multi_map类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class _Kty,
    class _Ty,
    class _Pr,
    class _Alloc>
struct TagTrait<std::multimap<_Kty, _Ty, _Pr, _Alloc> >
{
    enum { value = TAG_MAP };
};

/*------------------------------------------------------------------------------
 *描  述: 定义pair类型的特性
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<class _Ty1,
    class _Ty2>
struct TagTrait<std::pair<_Ty1, _Ty2> >
{
    enum { value = TAG_PAIR };
};

}  // namespace BroadCache

#endif  // HEADER_SERIALIZE_TAG
