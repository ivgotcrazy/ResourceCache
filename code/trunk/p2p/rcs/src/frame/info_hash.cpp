/*------------------------------------------------------------------------------
 * 文件名   : info_hash.cpp
 * 创建人   : rosan 
 * 创建时间 : 2013.09.17
 * 文件描述 : 实现InfoHash的子类
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 -----------------------------------------------------------------------------*/
#include "info_hash.hpp"
#include "bc_assert.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: InfoHash16构造函数
 * 参  数: [in] hash hash字符串
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
InfoHash16::InfoHash16(const std::string& hash) : hash_(hash)
{
    BC_ASSERT(hash_.size() == 16);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断两个InfoHash是否相等
 * 参  数: [in] info_hash 另一个InfoHash对象
 * 返回值: 两个info_hash对象是否相等
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool InfoHash16::Equal(const InfoHash& info_hash) const
{
    auto rhs = dynamic_cast<const InfoHash16*>(&info_hash); //判断是否是同类型的对象

    return (rhs != nullptr) && (hash_ == rhs->hash_);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取此对象的16进制字符串表示
 * 参  数: 
 * 返回值: 此对象的16进制字符串表示
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
std::string InfoHash16::to_string() const
{
    return ToHex(hash_);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取info-hash的原始数据
 * 参  数:
 * 返回值: info-hash的原始数据
 * 修  改:
 *   时间 2013.10.12
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
std::string InfoHash16::raw_data() const
{
    return hash_;
}

/*------------------------------------------------------------------------------
 * 描  述: InfoHash20的构造函数
 * 参  数: [in] hash hash字符串
 * 返回值:
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
InfoHash20::InfoHash20(const std::string& hash) : hash_(hash)
{
    BC_ASSERT(hash_.size() == 20);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断指定的InfoHash对象和本对象是否相等
 * 参  数: [in] info_hash 另一个info_hash对象
 * 返回值: 指定的InfoHash对象和本对象是否相等
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
bool InfoHash20::Equal(const InfoHash& info_hash) const
{
    auto rhs = dynamic_cast<const InfoHash20*>(&info_hash);
    return (rhs != nullptr) && (hash_ == rhs->hash_);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取此对象的16进制字符串表示
 * 参  数:
 * 返回值: 此对象的16进制字符串表示
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
std::string InfoHash20::to_string() const
{
    return ToHex(hash_);
}

/*------------------------------------------------------------------------------
 * 描  述: 获取info-hash的原始数据
 * 参  数:
 * 返回值: info-hash的原始数据
 * 修  改:
 *   时间 2013.10.12
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
std::string InfoHash20::raw_data() const
{
    return hash_;
}

}