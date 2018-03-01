/*#############################################################################
 * 文件名   : info_hash.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : InfoHash声明，包含接口声明和各派生类声明
 * ##########################################################################*/
#ifndef HEADER_INFO_HASH
#define HEADER_INFO_HASH

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "big_number.hpp"
#include "hash_stream.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: info-hash的基类
 *作  者: rosan
 *时  间: 2013.09.17
 -----------------------------------------------------------------------------*/
INTERFACE InfoHash
{
	virtual ~InfoHash() {}
   
    // 判断此对象是否和另一个InfoHash对象相等 
    virtual bool Equal(const InfoHash& info_hash) const = 0;

    // 获取此对象的原始数据
	virtual std::string to_string() const = 0;

    // 获取此对象16进制的字符串表示
    virtual std::string raw_data() const = 0;
};

/*------------------------------------------------------------------------------
 * 描  述: 判断两个InfoHash对象是否相等
 * 参  数: [in] lhs 第一个InfoHash对象
 *         [in] rhs 第二个InfoHash对象
 * 返回值: 两个InfoHash对象是否相等
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
inline bool operator==(const InfoHash& lhs, const InfoHash& rhs)
{
    return lhs.Equal(rhs);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断两个InfoHash对象是否不相等
 * 参  数: [in] lhs 第一个InfoHash对象
 *         [in] rhs 第二个InfoHash对象
 * 返回值: 两个InfoHash对象是否不相等
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/
inline bool operator!=(const InfoHash& lhs, const InfoHash& rhs)
{
    return !(lhs == rhs);
}

/*------------------------------------------------------------------------------
 * 描  述: 判断两个InfoHashSP对象是否相等
 * 参  数: [in] lhs 第一个InfoHashSP对象
 *         [in] rhs 第二个InfoHashSP对象
 * 返回值: 两个InfoHashSP对象是否相等
 * 修  改:
 *   时间 2013.10.31
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
inline bool operator==(const InfoHashSP& lhs, const InfoHashSP& rhs)
{
    return *lhs == *rhs;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算hash值，适配unorderd_map的hash函数
 * 参  数: [in] info_hash InfoHashSP
 * 返回值: hash值
 * 修  改:
 *   时间 2013年10月31日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/
inline std::size_t hash_value(const InfoHashSP& info_hash)
{
	HashStream hash_stream;
	hash_stream & info_hash->to_string();

	return hash_stream.value();
}

/*------------------------------------------------------------------------------
 *描  述: 16位的info-hash类
 *作  者: rosan
 *时  间: 2013.09.17
 -----------------------------------------------------------------------------*/
class InfoHash16 : public InfoHash
{
public:
    explicit InfoHash16(const std::string& hash);

    virtual bool Equal(const InfoHash& info_hash) const override;
    virtual std::string to_string() const override;
    virtual std::string raw_data() const override;

private:
    std::string hash_; //hash数据
};

/*------------------------------------------------------------------------------
 *描  述: 20位的info-hash类
 *作  者: rosan
 *时  间: 2013.09.17
 -----------------------------------------------------------------------------*/
class InfoHash20 : public InfoHash
{
public:
    explicit InfoHash20(const std::string& hash);

    virtual bool Equal(const InfoHash& info_hash) const override;
    virtual std::string to_string() const override; 
    virtual std::string raw_data() const override;

private:
    std::string hash_; //hash数据
};
 
typedef InfoHash20 BtInfoHash; //定义bt协议的info-hash
typedef InfoHash20 PpsInfoHash; //定义pps协议info_hash

}

#endif
