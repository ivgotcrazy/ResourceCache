/*#############################################################################
 * �ļ���   : bc_util.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��01��
 * �ļ����� : ȫ�ֹ���ʹ�ú���
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���ַ����л�ȡEndPoint
 *			  �㷨����:
 *			  1.�Ƚ����������ַ�����":"Ϊ�ָ��,�ָ�����ɶ�
 *			  2.����һ�ν���Ϊip��ַ
 *			  3.���ڶ��ν���Ϊport
 *			  4.����
 * ��  ��: [in] str ���������ַ���
 *         [out] endpoint ���ؽ�����endpoint
 * ����ֵ: �����Ƿ�ɹ� 
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 *        �����ĸ�ʽ��IP:PORT
 *************************************************************************/
bool ParseEndPoint(const std::string& str, EndPoint& endpoint)
{
    static const boost::regex kReg("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{4,5})");

    boost::smatch match;

    bool matched = boost::regex_match(str, match, kReg);
    if (matched)
    {
        endpoint.ip = to_address_ul(match[1]);  // ����һ����ת����ip��ַ
        endpoint.port = boost::lexical_cast<unsigned short>(match[2]);  // ���ڶ�����ת���ɶ˿ں�
    }

    return matched;
}

/**************************************************************************
 * ��  ��: ���ַ����л�ȡEndPoint�б�
 *			  �㷨����:
 *			  1.�����������ַ�����";"�ָ��,�ָ�����ɲ���
 *			     ����ÿһ������������"IP:PORT"��ʽ���ַ���
 *			  2.����ÿһ����,��ÿһ����ת����EndPoint����,����ӵ������б�
 *			  3.����
 * ��  ��: [in] str ���������ַ���
 * ����ֵ: EndPoint�б�
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 *        �����ĸ�ʽ��IP:PORT;IP:PORT;IP:PORT;IP:PORT
 *************************************************************************/
std::vector<EndPoint> ParseEndPoints(const std::string& str)
{
	// ���ַ����ָ��"IP:PORT"��ʽ
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
 * ��  ��: ��dynamic_bitset����ת�����ַ���
 * ��  ��: [in] bitset ��ת����dynamic_bitset����
 * ����ֵ: ת������ַ���
 * ��  ��:
 *   ʱ�� 2013.10.20
 *   ���� rosan
 *   ���� ����
 *************************************************************************/
std::string ToString(const boost::dynamic_bitset<>& bitset)
{
		std::string str;
		boost::to_string(bitset, str);

		return str;
}
 
/*------------------------------------------------------------------------------
 * ��  ��: ��ȡdynamic_bitset����ķ�ת
 *         �㷨���£�
 *         1.��ԭ����dynamic_bitset����ΪA����ת֮��Ķ���ΪB
 *         2.A��Bλ��Ŀ��ͬ����A.size() == B.size()
 *         3.����A��λ��ĿΪn����B�ĵ�iλ��ΪA�ĵ�n-i-1λ
 * ��  ��: bitset����ת�Ķ���
 * ����ֵ: dynamic_bitset����ķ�ת
 * ��  ��:
 *   ʱ�� 2013.10.25
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ��ʱ���ת��Ϊ����
 * ��  ��: [in] start ��ʼʱ��
 *         [in] end ����ʱ��
 * ����ֵ: ����������Ϊ����
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
int64 GetDurationSeconds(const ptime& start, const ptime& end)
{
	time_duration duration = end - start;
	return FromDurationToSeconds(duration);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ʱ���ת��Ϊ����
 * ��  ��: [in] duration ʱ���
 * ����ֵ: ����������Ϊ����
 * ��  ��:
 *   ʱ�� 2013��10��28��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
int64 FromDurationToSeconds(const time_duration& duration)
{
	int64 hours = duration.hours();
	int64 minutes = duration.minutes();
	int64 seconds = duration.seconds();

	return (hours * 3600 + minutes * 60 + seconds);
}

/*------------------------------------------------------------------------------
 * ��  ��: ������������ת����16�����ַ���
 * ��  ��: [in]data ��ת���Ķ���������
 *         [in]lower_case ת���ɵ�16�����ַ����е��ַ��Ƿ���Сд
 * ����ֵ: ת�����16�����ַ���
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::string ToHex(const std::string& data, bool lower_case)
{
    std::string hex_data; //16�����ַ���
    for (char ch : data)
    {
        hex_data.push_back(ToHex((ch >> 4) & 0xf, lower_case)); //����4λת����16�����ַ�
        hex_data.push_back(ToHex(ch & 0xf, lower_case)); //����4λת����16�����ַ�
    }

    return hex_data;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��16�����ַ���ת���ɶ���������
 * ��  ��: [in]hex_data ��ת����16�����ַ���
 * ����ֵ: ת����Ķ���������
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::string FromHex(const std::string& hex_data)
{
    std::string data; //����������

    auto size = hex_data.size(); //16�����ַ�������
    if ((size & 2) == 0) //�Ƿ���2��������(���������ݲ�����)
    {
        for (decltype(size) i = 0; i < size; i += 2)
        {
            //�����ŵ�2��16�����ַ��ϲ���һ���������ַ�
            data.push_back((FromHex(hex_data[i]) << 4) | FromHex(hex_data[i + 1]));
        }
    }

    return data;
}

/*------------------------------------------------------------------------------
 * ��  ��: ������������ת���ɷ���url����淶���ַ���
 *         ת���㷨��:
 *         �Ƚ��������е�ÿһ���ַ�ת��������16�����ַ�
 *         ����������16�ַ�ǰ�����ת���ַ�%
 * ��  ��: [in]data ��ת���Ķ���������
 * ����ֵ: ת����ķ���url����淶���ַ���
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::string ToHexUrl(const std::string& data)
{    
    std::string url_data; //url�����ַ���
    for (char ch : data)
    {
        url_data.push_back('%'); //���ת���ַ�
        url_data.push_back(ToHex((ch >> 4) & 0xf)); //���������ַ��ĸ�4λת����16�����ַ�
        url_data.push_back(ToHex(ch & 0xf)); //���������ַ���4λת����16�����ַ�
    }

    return url_data;
}

/*------------------------------------------------------------------------------
 * ��  ��: ������url����淶���ַ���ת���ɶ���������
 * ��  ��: [in]url_data ��ת���ķ���url����淶���ַ���
 * ����ֵ: ת����Ķ���������
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
std::string FromHexUrl(const std::string& url_data)
{
    std::string data;
    auto size = url_data.size();

    decltype(size) i = 0; 
    while (i < size)
    {
        if (IsNormalChar(url_data[i])) //�Ƿ���������url�����ַ�
        {
            data.push_back(url_data[i]); //����ת����ֱ�����
            ++i;
        } 
        else //��ת���ַ� % 
        {
            BC_ASSERT(url_data[i] == '%');
            BC_ASSERT(size - i > 2);

            //��ת���ַ�%�������16����ַ��ϲ���һ���������ַ?            
            data.push_back((FromHex(url_data[i + 1]) << 4) | FromHex(url_data[i + 2]));
            i += 3;
        }
    }

    return data;
}

/*------------------------------------------------------------------------------
 * ��  ��: �ָ��ַ���
 * ��  ��: [in] str ԭʼ�ַ���
 *         [in] delim �ָ���
 * ����ֵ: �ָ����ַ���
 * ��  ��:
 *   ʱ�� 2013��10��31��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �������ݵ�sha1ֵ
 * ��  ��: [in] data ����ָ�� 
 *         [in] length ���ݳ���
 * ����ֵ: ���ݵ�sha1ֵ��20λ
 * ��  ��:
 *   ʱ�� 2013.11.06
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ��ȡcrc16У���
 * ��  ��: [in] data ��У�������
 *         [in] byte_count ��У������ݳ���
 * ����ֵ: crc16У���
 * ��  ��:
 *   ʱ�� 2013.11.12
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
uint16 GetCrc16Checksum(const void* data, size_t byte_count)
{
    boost::crc_16_type crc16;
    crc16.process_bytes(data, byte_count);
    
    return uint16(crc16.checksum());
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡcrc32У���
 * ��  ��: [in] data ��У�������
 *         [in] byte_count ��У������ݳ���
 * ����ֵ: crc32У���
 * ��  ��:
 *   ʱ�� 2013.11.12
 *   ���� rosan
 *   ���� ����
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
 * ��  ��: ��URL�н���key=value��ʽ�ļ�ֵ��
 * ��  ��: [in] url URL
 * ����ֵ: ������ļ�ֵ��
 * ��  ��:
 *   ʱ�� 2013.11.16
 *   ���� rosan
 *   ���� ����
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
