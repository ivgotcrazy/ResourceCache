/*------------------------------------------------------------------------------
 * 文件名   : bencode_impl.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.09.13
 * 文件描述 : 此文件主要实现bencode的编码 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_BENCODE_IMPL
#define HEADER_BENCODE_IMPL

#include <cassert>
#include <algorithm>
#include <boost/lexical_cast.hpp>

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: 此类只是一个占位符，用以区分重载的函数
          带有ExtensionPlaceholder参数的函数不仅要编码它本身，
          还要附带编码它的前缀和后缀
 *作  者: rosan
 *时  间: 2013.09.13
 -----------------------------------------------------------------------------*/ 
class ExtensionPlaceholder {};

template<class Stream>
Stream& Bencode(Stream& stream, char value);
template<class Stream>
Stream& Bencode(Stream& stream, size_type value);
template<class Stream>
Stream& Bencode(Stream& stream, size_type value, ExtensionPlaceholder);
template<class Stream>
Stream& Bencode(Stream& stream, const std::string& value);
template<class Stream>
Stream& Bencode(Stream& stream, const std::string& value, ExtensionPlaceholder);
template<class Stream>
Stream& Bencode(Stream& stream, const BencodeEntry& value);
template<class Stream>
Stream& Bencode(Stream& stream, const std::list<BencodeEntry>& value);
template<class Stream>
Stream& Bencode(Stream& stream, const std::map<std::string, BencodeEntry>& value);

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个字符
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的字符
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
inline Stream& Bencode(Stream& stream, char value)
{
    return stream += value;
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个整数
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的整数
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
inline Stream& Bencode(Stream& stream, size_type value)
{
    return Bencode(stream, boost::lexical_cast<std::string>(value));  // 先将整数转换成字符串，然后bencode编码字符串
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个整数
 *			  算法如下:
 *			  1.编码整数前缀"i"
 *			  2.将整数转换成字符串并编码
 *			  3.编码整数后缀"e"
 *			  4.返回
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的整数
 *         [in] ExtensionPlaceholder 占位符，
 *         用于指示需要编码整数的前缀i和后缀e
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
inline Stream& Bencode(Stream& stream, size_type value, ExtensionPlaceholder)
{
    Bencode(stream, 'i');
    Bencode(stream, value);
    Bencode(stream, 'e');

    return stream;
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个字符串
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的字符串
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
inline Stream& Bencode(Stream& stream, const std::string& value)
{
    return stream += value;
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个字符串
 *			  算法如下:
 *			  1.将输入字符串的长度转换成字符串并编码
 *			  2.编码字符串分割符":"
 *			  3.编码输入字符串本身
 *			  4.返回
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的字符串
 *         ExtensionPlaceholder 占位符，用于指示需要
 *         编码字符串的前缀，即字符串的长度和":"
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
inline Stream& Bencode(Stream& stream, const std::string& value, ExtensionPlaceholder)
{
    Bencode(stream, static_cast<size_type>(value.size()));
    Bencode(stream, ':');
    Bencode(stream, value);

    return stream;
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个BencodeEntry对象
 *			  算法如下:
 *			  1.判断BencodeEntry对象是否int, string, list, dictionary之一
 *			    如果不是跳到步骤3
 *			  2.根据BencodeEntry实际保存的数据类型,并调用相应的编码函数
 *			  3.返回
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的BencodeEntry对象
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 *        这个是bencode编码的总入口，根据BencodeEntry实际保存的类型，
 *        然后调用相应的函数
 -----------------------------------------------------------------------------*/ 
template<class Stream>
Stream& Bencode(Stream& stream, const BencodeEntry& value)
{
    switch(value.Type())
    {
    case BencodeEntry::INT_T : return Bencode(stream, value.Integer(), ExtensionPlaceholder());
    case BencodeEntry::STRING_T : return Bencode(stream, value.String(), ExtensionPlaceholder());
    case BencodeEntry::LIST_T : return Bencode(stream, value.List());
    case BencodeEntry::DICTIONARY_T : return Bencode(stream, value.Dict());
    default : return stream;
    }
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个BencodeEntry列表
 *			  算法如下:
 *			  1.编码list前缀"i"
 *			  2.遍历list的每一个元素并编码
 *			  3.编码list后缀"e"
 *			  4.返回
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的BencodeEntry列表
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
Stream& Bencode(Stream& stream, const std::list<BencodeEntry>& value)
{
    Bencode(stream, 'l'); //编码list前缀

    for(const BencodeEntry& entry : value)
    {
        Bencode(stream, entry);
    }

    Bencode(stream, 'e'); //编码list后缀

    return stream;
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个字典类型对象
 *			  算法如下:
 *			  1.编码dictionary前缀"d"
 *			  2.遍历dictionary的每一个元素
 *				 2.1编码dictionary每一个元素的key
 *			     2.2编码dictionary每一个元素的value
 *			  3.编码dictionary后缀"e"
 *			  4.返回
 * 参  数: [in][out] stream 输出流对象 
 *         [in] value 欲编码的字典
 * 返回值: 输出流对象
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Stream>
Stream& Bencode(Stream& stream, const std::map<std::string, BencodeEntry>& value)
{
    Bencode(stream, 'd'); //编码字典前缀

    for(decltype(*value.begin()) item : value)
    {
        Bencode(stream, item.first, ExtensionPlaceholder()); //编码字典key
        Bencode(stream, item.second); //编码字典value
    }

    Bencode(stream, 'e'); //编码字典后缀 
    
    return stream;
}

/*------------------------------------------------------------------------------
 * 描  述: bencode编码一个BencodeEntry对象
 * 参  数: [in] entry BencodeEntry对象 
 * 返回值: 编码后的字符串 
 * 修  改: 
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline std::string Bencode(const BencodeEntry& entry)
{
    std::string stream;
    Bencode(stream, entry);

    return stream;
}

}

#endif  // HEADER_BENCODE_IMPL
