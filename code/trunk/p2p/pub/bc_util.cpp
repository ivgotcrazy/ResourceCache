/*#############################################################################
 * 文件名   : bc_util.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月01日
 * 文件描述 : 全局公共使用函数
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bc_util.hpp"
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/crc.hpp>
#include "bc_assert.hpp"
#include "endpoint.hpp"
#include "sha1.hpp"

namespace BroadCache
{

/**************************************************************************
 * 描  述: 从字符串中获取EndPoint
 *			  算法如下:
 *			  1.先将待解析的字符串以":"为分割符,分割成若干段
 *			  2.将第一段解析为ip地址
 *			  3.将第二段解析为port
 *			  4.返回
 * 参  数: [in] str 待解析的字符串
 *         [out] endpoint 返回解析的endpoint
 * 返回值: 解析是否成功 
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 *        解析的格式是IP:PORT
 *************************************************************************/
bool ParseEndPoint(const std::string& str, EndPoint& endpoint)
{
    static const boost::regex kReg("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{4,5})");

    boost::smatch match;

    bool matched = boost::regex_match(str, match, kReg);
    if (matched)
    {
        endpoint.ip = to_address_ul(match[1]);  // 将第一部分转换成ip地址
        endpoint.port = boost::lexical_cast<unsigned short>(match[2]);  // 将第二部分转换成端口号
    }

    return matched;
}

/**************************************************************************
 * 描  述: 从字符串中获取EndPoint列表
 *			  算法如下:
 *			  1.将待解析的字符串用";"分割符,分割成若干部分
 *			     其中每一部分理论上是"IP:PORT"格式的字符串
 *			  2.遍历每一部分,将每一部分转换成EndPoint对象,并添加到返回列表
 *			  3.返回
 * 参  数: [in] str 待解析的字符串
 * 返回值: EndPoint列表
 * 修  改:
 *   时间 2013.09.17
 *   作者 rosan
 *   描述 创建
 *        解析的格式是IP:PORT;IP:PORT;IP:PORT;IP:PORT
 *************************************************************************/
std::vector<EndPoint> ParseEndPoints(const std::string& str)
{
	// 将字符串分割成"IP:PORT"格式
	std::vector<std::string> str_vec = SplitStr(str, ' ');

    EndPoint endpoint;
	std::vector<EndPoint> endpoints;
	
	for (const std::string& ep_str : str_vec)
	{
        if (ParseEndPoint(ep_str, endpoint))
        {
            endpoints.push_back(endpoint);
        }
	}

    return endpoints;
}

/**************************************************************************
 * 描  述: 将dynamic_bitset对象转换成字符串
 * 参  数: [in] bitset 被转换的dynamic_bitset对象
 * 返回值: 转换后的字符串
 * 修  改:
 *   时间 2013.10.20
 *   作者 rosan
 *   描述 创建
 *************************************************************************/
std::string ToString(const boost::dynamic_bitset<>& bitset)
{
		std::string str;
		boost::to_string(bitset, str);

		return str;
}
 
/*------------------------------------------------------------------------------
 * 描  述: 获取dynamic_bitset对象的反转
 *         算法如下：
 *         1.记原来的dynamic_bitset对象为A，反转之后的对象为B
 *         2.A和B位数目相同，即A.size() == B.size()
 *         3.假设A的位数目为n，则B的第i位即为A的第n-i-1位
 * 参  数: bitset被反转的对象
 * 返回值: dynamic_bitset对象的反转
 * 修  改:
 *   时间 2013.10.25
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
boost::dynamic_bitset<> GetReversedBitset(const boost::dynamic_bitset<>& bitset)
{
     size_t size = bitset.size();
     boost::dynamic_bitset<> bitset_reversed(size);
 
     for(size_t i = 0; i < size; ++i)
     {
         bitset_reversed[i] = bitset[size - i - 1];
     }
 
     return bitset_reversed;
}

/*------------------------------------------------------------------------------
 * 描  述: 将时间段转换为秒数
 * 参  数: [in] start 开始时间
 *         [in] end 结束时间
 * 返回值: 秒数，可能为负数
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
int64 GetDurationSeconds(const ptime& start, const ptime& end)
{
	time_duration duration = end - start;
	return FromDurationToSeconds(duration);
}

/*------------------------------------------------------------------------------
 * 描  述: 将时间段转换为秒数
 * 参  数: [in] duration 时间段
 * 返回值: 秒数，可能为负数
 * 修  改:
 *   时间 2013年10月28日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
int64 FromDurationToSeconds(const time_duration& duration)
{
	int64 hours = duration.hours();
	int64 minutes = duration.minutes();
	int64 seconds = duration.seconds();

	return (hours * 3600 + minutes * 60 + seconds);
}

/*------------------------------------------------------------------------------
 * 描  述: 将二进制数据转换成16进制字符串
 * 参  数: [in]data 被转换的二进制数据
 *         [in]lower_case 转换成的16进制字符串中的字符是否是小写
 * 返回值: 转换后的16进制字符串
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::string ToHex(const std::string& data, bool lower_case)
{
    std::string hex_data; //16进制字符串
    for (char ch : data)
    {
        hex_data.push_back(ToHex((ch >> 4) & 0xf, lower_case)); //将高4位转换成16进制字符
        hex_data.push_back(ToHex(ch & 0xf, lower_case)); //将低4位转换成16进制字符
    }

    return hex_data;
}

/*------------------------------------------------------------------------------
 * 描  述: 将16进制字符串转换成二进制数据
 * 参  数: [in]hex_data 被转换的16进制字符串
 * 返回值: 转换后的二进制数据
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::string FromHex(const std::string& hex_data)
{
    std::string data; //二进制数据

    auto size = hex_data.size(); //16进制字符串长度
    if ((size & 2) == 0) //是否是2的整数倍(不是则数据不正常)
    {
        for (decltype(size) i = 0; i < size; i += 2)
        {
            //将挨着的2个16进制字符合并成一个二进制字符
            data.push_back((FromHex(hex_data[i]) << 4) | FromHex(hex_data[i + 1]));
        }
    }

    return data;
}

/*------------------------------------------------------------------------------
 * 描  述: 将二进制数据转换成符合url编码规范的字符串
 *         转换算法是:
 *         先将二进制中的每一个字符转换成两个16进制字符
 *         再在这两个16字符前面加上转译字符%
 * 参  数: [in]data 被转换的二进制数据
 * 返回值: 转换后的符合url编码规范的字符串
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::string ToHexUrl(const std::string& data)
{    
    std::string url_data; //url编码字符串
    for (char ch : data)
    {
        url_data.push_back('%'); //添加转译字符
        url_data.push_back(ToHex((ch >> 4) & 0xf)); //将二进制字符的高4位转换成16进制字符
        url_data.push_back(ToHex(ch & 0xf)); //将二进制字符的4位转换成16进制字符
    }

    return url_data;
}

/*------------------------------------------------------------------------------
 * 描  述: 将符合url编码规范的字符串转换成二进制数据
 * 参  数: [in]url_data 被转换的符合url编码规范的字符串
 * 返回值: 转换后的二进制数据
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::string FromHexUrl(const std::string& url_data)
{
    std::string data;
    auto size = url_data.size();

    decltype(size) i = 0; 
    while (i < size)
    {
        if (IsNormalChar(url_data[i])) //是否是正常的url编码字符
        {
            data.push_back(url_data[i]); //不用转换，直接输出
            ++i;
        } 
        else //是转译字符 % 
        {
            BC_ASSERT(url_data[i] == '%');
            BC_ASSERT(size - i > 2);

            //将转译字符%后的两个16进制址喜⒊梢桓龆谱址?            
            data.push_back((FromHex(url_data[i + 1]) << 4) | FromHex(url_data[i + 2]));
            i += 3;
        }
    }

    return data;
}

/*------------------------------------------------------------------------------
 * 描  述: 分割字符串
 * 参  数: [in] str 原始字符串
 *         [in] delim 分隔符
 * 返回值: 分割后的字符串
 * 修  改:
 *   时间 2013年10月31日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::vector<std::string> SplitStr(const std::string& str, char delim)
{
	std::vector<std::string> str_vec;
	std::string::size_type start_pos = 0, end_pos = 0;
	std::string tmp;

	while (start_pos < str.size())
	{
		end_pos = str.find(delim, start_pos);
		if (end_pos == std::string::npos)
		{
			tmp = str.substr(start_pos);
			str_vec.push_back(tmp);
			break;
		}
		
		if (start_pos != end_pos) 
		{
			tmp = str.substr(start_pos, end_pos - start_pos);
			str_vec.push_back(tmp);
		}

		start_pos = end_pos + 1;
	}

	return str_vec;
}

/*------------------------------------------------------------------------------
 * 描  述: 计算数据的sha1值
 * 参  数: [in] data 数据指针 
 *         [in] length 数据长度
 * 返回值: 数据的sha1值，20位
 * 修  改:
 *   时间 2013.11.06
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::string GetSha1(const char* data, uint32 length)
{
    SHA_CTX context;
    std::string digest(20, '\0');
    
    SHA1_Init(&context);
    SHA1_Update(&context, reinterpret_cast<const unsigned char*>(data), length);
    SHA1_Final((unsigned char*)(digest.c_str()), &context);
    
    return digest; 
}

/*------------------------------------------------------------------------------
 * 描  述: 获取crc16校验和
 * 参  数: [in] data 待校验的数据
 *         [in] byte_count 待校验的数据长度
 * 返回值: crc16校验和
 * 修  改:
 *   时间 2013.11.12
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
uint16 GetCrc16Checksum(const void* data, size_t byte_count)
{
    boost::crc_16_type crc16;
    crc16.process_bytes(data, byte_count);
    
    return uint16(crc16.checksum());
}

/*------------------------------------------------------------------------------
 * 描  述: 获取crc32校验和
 * 参  数: [in] data 待校验的数据
 *         [in] byte_count 待校验的数据长度
 * 返回值: crc32校验和
 * 修  改:
 *   时间 2013.11.12
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
uint32 GetCrc32Checksum(const void* data, size_t byte_count)
{
    boost::crc_32_type crc32;
    crc32.process_bytes(data, byte_count);
    
    return uint32(crc32.checksum());
}

boost::posix_time::ptime GetFileLastReadTime(const std::string path)
{
	struct stat buf;
    stat(path.c_str(), &buf);
	
    LOG(INFO) << "Get Last Read Time | value:" << buf.st_atime;
	
	return  boost::posix_time::from_time_t(buf.st_atime);
}

uint64 CalcDirectorySize(const fs::path path)
{
	uint64  total_size = 0;
	if (fs::is_directory(path))
	{
		fs::directory_iterator item(path);
		fs::directory_iterator item_end;
		for ( ; item != item_end; item++)
		{
			uint64 size =0;
			if (fs::is_directory(item->path()))
			{
			   size = CalcDirectorySize(item->path());
			}
			else
			{	
			   size = fs::file_size(item->path());
			}
			total_size += size;
		}
	}
	else 
	{
		total_size = fs::file_size(path);
	}
	
	return total_size;
}

/*------------------------------------------------------------------------------
 * 描  述: 从URL中解析key=value格式的键值对
 * 参  数: [in] url URL
 * 返回值: 解析后的键值对
 * 修  改:
 *   时间 2013.11.16
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
std::map<std::string, std::string> ParseKeyValuePairs(const std::string& url)
{
    static const boost::regex kRegexPairs("(?:([^&]+)=([^&]+)&)*([^&]+)=([^&]+)");

    std::map<std::string, std::string> m;

    auto p1 = url.find('?');
    if ((p1 == 0) || (p1 == std::string::npos) || (p1 + 1 == url.size()))
        return m;

    auto p2 = url.find(' ', p1);
    if (p2 == std::string::npos)
        return m;

    std::string parameter = url.substr(p1 + 1, p2 - p1 - 1);
	
    boost::smatch what;
    if (boost::regex_match(std::string(parameter), what, kRegexPairs, boost::match_extra))
    {
        auto& keys = what.captures(1);
        auto& values = what.captures(2);

        for (decltype(keys.size()) i = 0; i < keys.size(); ++i)
        {
            m[keys[i]] = values[i]; 
        }

		m[what[3]] = what[4];
    }

    return m;
}

}
