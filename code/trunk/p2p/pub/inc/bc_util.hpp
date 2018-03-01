/*#############################################################################
 * �ļ���   : bc_util.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��01��
 * �ļ����� : ȫ�ֹ���ʹ�ú���
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
//================================ ��ͨ��������=================================

// ���ַ����н�����IP:PORT
void ParseEndPointsFromStr(std::string str, std::vector<EndPoint>& ep_vec);  
bool ParseEndPoint(const std::string& str, EndPoint& endpoint);
std::vector<EndPoint> ParseEndPoints(const std::string& str);

// ��dynamic_bitset����ת�����ַ���
std::string ToString(const boost::dynamic_bitset<>& bitset);  

// ��ȡdynamic_bitsetλ����
boost::dynamic_bitset<> GetReversedBitset(const boost::dynamic_bitset<>& bitset);

// ����ʱ�������
int64 FromDurationToSeconds(const time_duration& duration);
int64 GetDurationSeconds(const ptime& start, const ptime& end);

// ��������16����ת��
std::string ToHex(const std::string& data, bool lower_case = false);
std::string FromHex(const std::string& hex_data);

// ��������url�����ַ���ת��
std::string ToHexUrl(const std::string& data);
std::string FromHexUrl(const std::string& url_data);

// �ָ��ַ���
std::vector<std::string> SplitStr(const std::string& str, char delim);

// �������ݵ�sha1ֵ
std::string GetSha1(const char* data, uint32 length);

// ����ļ����ķ���ʱ��
boost::posix_time::ptime GetFileLastReadTime(const std::string path);

uint64 CalcDirectorySize(const fs::path path);


//============================== inline��������=================================

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
 * ��  ��: ��һ���ַ���4λ(��4λ���4λ)ת����16���Ƶ��ַ�
 * ��  ��: [in] ch �ַ���4λ(��4λ���4λ)
 *         [in] lower_case �Ƿ���ת����16���Ƶ�Сд�ַ�
 * ����ֵ: ת�����16�����ַ�
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline char ToHex(char ch, bool lower_case = false)
{
    static const char kTable[2][16] = {
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'},
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'} };

    return kTable[!lower_case][ch & 0xf];          
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��16���Ƶ��ַ�ת����4λ���ַ�
 * ��  ��: [in] ch ��ת����16�����ַ�
 * ����ֵ: ת�����4λ�ַ�
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: �ж�һ���ַ���url�������Ƿ��ǺϷ����ַ�
 *         һ�㲻�ɴ�ӡ�ĺ�url�������⺬���ַ����ǷǷ��ģ��������ת��
 * ��  ��: [in] ch ���жϵ��ַ�
 * ����ֵ: �Ƿ��ǺϷ���url�ַ�
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
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

// ��ȡcrcУ���
uint16 GetCrc16Checksum(const void* data, size_t byte_count);
uint32 GetCrc32Checksum(const void* data, size_t byte_count);

// ��URL�н���key=value��ʽ�ļ�ֵ��
std::map<std::string, std::string> ParseKeyValuePairs(const std::string& url);

// ��һ���ṹ���е���������
template<class StructType>
inline void ClearStruct(StructType& st)
{
    memset(&st, 0, sizeof(StructType));
}

// ��ȡһ����պ�Ľṹ��
template<class StructType>
inline StructType GetClearedStruct()
{
    StructType st;
    ClearStruct(st);
    
    return st;
}

// �������һ��ʵ��
template<typename RealType>
inline RealType RandomUniformReal(RealType min = 0.0, RealType max = 1.0)
{
    static boost::mutex mutex;
    static boost::random::mt19937 generator(time(nullptr));

    boost::lock_guard<decltype(mutex)> lock(mutex);
    boost::random::uniform_real_distribution<RealType> distribution(min, max);

    return distribution(generator);
}

// �������һ������
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

// ���ѡ��һ�������е����ɸ�Ԫ��
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

// ɾ�������е�һ��Ԫ�أ������ظ�Ԫ�ص���һ��λ��
template<class Container>
inline typename Container::iterator Erase(Container& container, 
                                          typename Container::iterator where)
{
    typename Container::iterator i = where;

    ++i;
    container.erase(where);

    return i;
}

//=================================== �궨�� ===================================

// ������������ı���
#define CALC_FULL_MULT(total, base) (((total) + (base) - 1) / (base))

// ���������е�Ԫ��
#define FOREACH(element, container) for (decltype(*container.begin()) element : container)

// �������ļ��л�ȡ�ַ���
#define GET_CONFIG_STR(ConfigParser, item, para) \
{ \
	ConfigParser& configer = ConfigParser::GetInstance(); \
	bool ret = configer.GetValue<std::string>(item, (para)); \
	if (!ret) \
	{ \
		LOG(FATAL) << "Parse config error | " << item; \
	} \
} \

// �������ļ��л�ȡboolֵ
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

// �������ļ��л�ȡ����
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
