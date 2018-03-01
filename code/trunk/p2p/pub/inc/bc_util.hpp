/*#############################################################################
 * 文件名   : bc_util.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月01日
 * 文件描述 : 全局公共使用函数
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BC_UTIL
#define HEADER_BC_UTIL

#include <string>
#include <vector>

#include <boost/random.hpp>
#include <boost/dynamic_bitset.hpp>

#include "bc_typedef.hpp"
#include "endpoint.hpp"

namespace BroadCache
{
//================================ 普通函数定义=================================

// 从字符串中解析出IP:PORT
void ParseEndPointsFromStr(std::string str, std::vector<EndPoint>& ep_vec);  
bool ParseEndPoint(const std::string& str, EndPoint& endpoint);
std::vector<EndPoint> ParseEndPoints(const std::string& str);

// 将dynamic_bitset对象转换成字符串
std::string ToString(const boost::dynamic_bitset<>& bitset);  

// 获取dynamic_bitset位反向
boost::dynamic_bitset<> GetReversedBitset(const boost::dynamic_bitset<>& bitset);

// 计算时间段秒数
int64 FromDurationToSeconds(const time_duration& duration);
int64 GetDurationSeconds(const ptime& start, const ptime& end);

// 二进制与16进制转换
std::string ToHex(const std::string& data, bool lower_case = false);
std::string FromHex(const std::string& hex_data);

// 二进制与url编码字符串转换
std::string ToHexUrl(const std::string& data);
std::string FromHexUrl(const std::string& url_data);

// 分割字符串
std::vector<std::string> SplitStr(const std::string& str, char delim);

// 计算数据的sha1值
std::string GetSha1(const char* data, uint32 length);

// 获得文件最后的访问时间
boost::posix_time::ptime GetFileLastReadTime(const std::string path);

uint64 CalcDirectorySize(const fs::path path);


//============================== inline函数定义=================================

inline ptime TimeNow() 
{ 
	return boost::date_time::second_clock<ptime>::universal_time(); 
}

inline bool IsDigit(char c) 
{ 
	return c >= '0' && c <= '9'; 
}

inline bool IsPrint(char c) 
{ 
	return c >= 32 && c < 127; 
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个字符的4位(低4位或高4位)转换成16进制的字符
 * 参  数: [in] ch 字符的4位(低4位或高4位)
 *         [in] lower_case 是否是转换成16进制的小写字符
 * 返回值: 转换后的16进制字符
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline char ToHex(char ch, bool lower_case = false)
{
    static const char kTable[2][16] = {
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'},
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'} };

    return kTable[!lower_case][ch & 0xf];          
}

/*------------------------------------------------------------------------------
 * 描  述: 将一个16进制的字符转换成4位的字符
 * 参  数: [in] ch 被转换的16进制字符
 * 返回值: 转换后的4位字符
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline char FromHex(char ch)
{
    static const char kTable[128] = {
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x00
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x10
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x20
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, //0x30 
         0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x40
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x50
         0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x60
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //0x70

    return kTable[ch & 0x7f];  
}

/*------------------------------------------------------------------------------
 * 描  述: 判断一个字符在url编码中是否是合法的字符
 *         一般不可打印的和url中有特殊含义字符都是非法的，必须进行转译
 * 参  数: [in] ch 被判断的字符
 * 返回值: 是否是合法的url字符
 * 修  改:
 *   时间 2013.10.14
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
inline char IsNormalChar(char ch)
{
    static const char kTable[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, //0x20
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, //0x30
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x40
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, //0x50
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x60
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0}; //0x70

    return kTable[ch & 0x7f]; 
}

// 获取crc校验和
uint16 GetCrc16Checksum(const void* data, size_t byte_count);
uint32 GetCrc32Checksum(const void* data, size_t byte_count);

// 从URL中解析key=value格式的键值对
std::map<std::string, std::string> ParseKeyValuePairs(const std::string& url);

// 将一个结构体中的数据清零
template<class StructType>
inline void ClearStruct(StructType& st)
{
    memset(&st, 0, sizeof(StructType));
}

// 获取一个清空后的结构体
template<class StructType>
inline StructType GetClearedStruct()
{
    StructType st;
    ClearStruct(st);
    
    return st;
}

// 随机生成一个实数
template<typename RealType>
inline RealType RandomUniformReal(RealType min = 0.0, RealType max = 1.0)
{
    static boost::mutex mutex;
    static boost::random::mt19937 generator(time(nullptr));

    boost::lock_guard<decltype(mutex)> lock(mutex);
    boost::random::uniform_real_distribution<RealType> distribution(min, max);

    return distribution(generator);
}

// 随机生成一个整数
template<typename IntType>
inline IntType RandomUniformInt(IntType min = 0, 
                                IntType max = (std::numeric_limits<IntType>::max)())
{
    static boost::mutex mutex;
    static boost::random::mt19937 generator(time(nullptr));

    boost::lock_guard<decltype(mutex)> lock(mutex);
    boost::random::uniform_int_distribution<IntType> distribution(min, max);

    return distribution(generator);
}

// 随机选择一个序列中的若干个元素
template<class Iter>
std::vector<Iter> RandomSelect(Iter begin, Iter end, uint32 num_want)
{
    std::vector<Iter> v;

    uint32 size = std::distance(begin, end);
    for (; begin != end; ++begin)
    {
        if (RandomUniformInt<decltype(num_want)>(1, size) <= num_want)
        {
            v.push_back(begin);            
            --num_want;
        }
        
        --size;
    }

    return v;
}

// 删除容器中的一个元素，并返回该元素的下一个位置
template<class Container>
inline typename Container::iterator Erase(Container& container, 
                                          typename Container::iterator where)
{
    typename Container::iterator i = where;

    ++i;
    container.erase(where);

    return i;
}

//=================================== 宏定义 ===================================

// 计算包含余数的倍数
#define CALC_FULL_MULT(total, base) (((total) + (base) - 1) / (base))

// 遍历容器中的元素
#define FOREACH(element, container) for (decltype(*container.begin()) element : container)

// 从配置文件中获取字符串
#define GET_CONFIG_STR(ConfigParser, item, para) \
{ \
	ConfigParser& configer = ConfigParser::GetInstance(); \
	bool ret = configer.GetValue<std::string>(item, (para)); \
	if (!ret) \
	{ \
		LOG(FATAL) << "Parse config error | " << item; \
	} \
} \

// 从配置文件中获取bool值
#define GET_CONFIG_BOOL(ConfigParser, item, para) \
{ \
	ConfigParser& configer = ConfigParser::GetInstance(); \
	std::string config_str; \
	bool ret = configer.GetValue<std::string>(item, config_str); \
	if (!ret || (config_str != "yes" && config_str != "no")) \
	{ \
		LOG(FATAL) << "Parse config error | " << item; \
	} \
	else if (config_str == "yes") \
	{ \
		para = true; \
	} \
	else \
	{ \
		para = false; \
	} \
} \

// 从配置文件中获取整形
#define GET_CONFIG_INT(ConfigParser, item, para) \
{ \
	ConfigParser& configer = ConfigParser::GetInstance(); \
	bool ret = configer.GetValue<uint32>(item, (para)); \
	if (!ret) \
	{ \
		LOG(FATAL) << "Parse config error | " << item; \
	} \
} \

}  // namespace BroadCache

#endif
