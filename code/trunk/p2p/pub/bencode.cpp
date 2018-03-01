/*------------------------------------------------------------------------------
 * 文件名   : bencode.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.09.13
 * 文件描述 : 实现bencode解码 
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 -----------------------------------------------------------------------------*/ 
#include "bencode.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: 此类代表一个常量的内存区间
 *作  者: rosan
 *时  间: 2013.09.13
 -----------------------------------------------------------------------------*/ 
struct ConstBufferInterval
{
	ConstBufferInterval() : beg(nullptr), end(nullptr) {}

    explicit ConstBufferInterval(const std::string& s) : beg(s.c_str()), 
        end(s.c_str() + s.size()) {}

	ConstBufferInterval(const char* b, const char* e) : beg(b), end(e) {}

    inline bool IsEmpty() const { return beg > end; } // 判断区间是否为空

    const char* beg;  // 区间开始
    const char* end;  // 区间结束 
};

/*------------------------------------------------------------------------------
 * 描  述: bencode解码出一个整数
 *         算法如下:
 *         1.先找到整数对应字符串的结束字符
 *         2.设置区间新的起始位置
 *         3.将整数对应的字符串转换成整数
 *         4.返回
 * 参  数: [in][out] interval 缓冲区区间
 *         [in] end_of_string 整数的结束字符
 * 返回值: 解码出的整数
 * 修  改:
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static size_type BdecodeInteger(ConstBufferInterval& interval, char end_of_string)
{
    auto j = interval.beg;  // 区间开始位置
    auto i = std::find(interval.beg, interval.end, end_of_string);  // 找到整数的结束字符

    BC_ASSERT((i > interval.beg) && (i < interval.end)); 

    interval.beg = i + 1;  // 重新设置缓冲区起点

    return boost::lexical_cast<size_type>(std::string(j, i - j)); 
}

/*------------------------------------------------------------------------------
 * 描  述: bencode解码出一个字符串
 * 参  数: [in][out]interval 缓冲区的区间
 *         [in]size 解码出的字符串的长度
 * 返回值: 解码出的字符串
 * 修  改:
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static std::string BdecodeString(ConstBufferInterval& interval, size_type size)
{
    BC_ASSERT(interval.end - interval.beg >= size);

    interval.beg += size; //重新设置缓冲区起点

    return std::string(interval.beg - size, size);
}

/*------------------------------------------------------------------------------
 * 描  述: 从缓冲区中解码出BencodeEntry对象
 *         算法如下:
 *         1.判断待解析的字符串区间是否为空,如果是则跳到步骤7
 *         2.获取字符串区间的第一个字符
 *         3.判断此字符是否是"i"
 *           如果是则解析一个整数,跳到步骤7
 *         4.判断此字符是否是"l"
 *           如果是则解析一个列表,跳到步骤7
 *         5.判断此字符是否是"d"
 *           如果是则解析一个字典,跳到步骤7
 *         6.解析一个字符串
 *         7.返回
 * 参  数: [in][out] interval
 * 返回值: 解码出的BencodeEntry对象
 * 修  改:
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static BencodeEntry Bdecode(ConstBufferInterval& interval)
{
    auto& beg = interval.beg;
    while (!interval.IsEmpty())
    {
        switch (*beg)  // 判断bencode类型
        {
        case 'i' :  // 整数
            ++beg;  // 跳过i字符
            return BencodeEntry(BdecodeInteger(interval, 'e')); 

        case 'l' :  // 列表
            {
                ++beg;  // 跳过l字符
                std::list<BencodeEntry> entry_list;
                while(*beg != 'e')  // 判断是否到了列表的尾部
                {
                    entry_list.push_back(Bdecode(interval));  // 解码出一个BencodeEntry加到列表中
                }
                ++beg;  // 跳过e字符
                return BencodeEntry(entry_list); 
            }

        case 'd' :  // 字典
            {
                ++beg;  // 跳过d字符
                std::map<std::string, BencodeEntry> entry_dict; 
                while (*beg != 'e')  // 判断是否到了字典的尾部
                { 
                    std::string key = Bdecode(interval).String();  // 解码出key
                    BencodeEntry value = Bdecode(interval);  // 解码出value
                    entry_dict.insert(std::make_pair(key, value)); //在字典中增加一项
                }
                ++beg;  // 跳过e字符
                return BencodeEntry(entry_dict);
            }

        default :  // 默认为字符串
            // 先解码出字符串的长度，然后解码出字符串
            return BencodeEntry(BdecodeString(interval, BdecodeInteger(interval, ':'))); 
        }
    }

    return BencodeEntry();
}

/*------------------------------------------------------------------------------
 * 描  述: 从缓冲区中解码出BencodeEntry对象
 * 参  数: [in] begin 缓冲区的开始
 *         [in] end 缓冲区的结束
 *         [out] len 返回编码使用的字节数目
 * 返回值: 解码出的BencodeEntry对象
 * 修  改:
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
BencodeEntry Bdecode(const char* begin, const char* end, size_t* len)
{
    ConstBufferInterval interval(begin, end);
    BencodeEntry entry = Bdecode(interval);

    if (len != nullptr)
    {
        *len = interval.beg - begin;
    }
    
    return entry;
}

/*------------------------------------------------------------------------------
 * 描  述: 从缓冲区中解码出BencodeEntry对象
 * 参  数: [in] data 数据缓冲区
 * 返回值: 解码出的BencodeEntry对象
 * 修  改:
 *   时间 2013.09.13
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
BencodeEntry Bdecode(const std::string& data)
{
    return data.empty() ? BencodeEntry() : Bdecode(data.c_str(), data.c_str() + data.size());
}

}
