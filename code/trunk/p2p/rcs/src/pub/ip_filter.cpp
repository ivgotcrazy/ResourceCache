#include "ip_filter.hpp"
#include <boost/utility.hpp>

namespace fs = boost::filesystem;	

namespace BroadCache
{

typedef std::string SpeedLevel;  // �ϴ��������ٶȼ���
typedef unsigned int SpeedLimit;  // �ϴ��������ٶȼ���
typedef std::map<SpeedLevel, SpeedLimit> SpeedLevels;  // �ϴ��������ٶȼ����Ӧ�ٶ���ֵ

/*------------------------------------------------------------------------------
 * ��  ��: ������������ֵת���ɷ���Ȩ��
 * ��  ��: [in] access_level ����������ֵ
 * ����ֵ: ����Ȩ��
 * ��  ��:
 *   ʱ�� 2013.11.02
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static AccessFlag ToAccessFlag(int access_level)
{
    if (access_level < 100)
    {
        return BLOCKED;
    }
    else if (access_level >= 200)
    {
        return INNER;
    }
    else
    {
        return OUTER;
    } 
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡĳһ��������ϴ�/�����ٶ�
 * ��  ��: [in] speed_levels ÿһ�������Ӧ���ٶ��ֵ�
 *         [in] level �ٶȼ���  
 * ����ֵ: �˼����Ӧ���ٶ�
 * ��  ��:
 *   ʱ�� 2013.11.02
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static SpeedLimit GetSpeedLimit(const SpeedLevels& speed_levels, const SpeedLevel& level)
{ 
    auto i = speed_levels.find(level);
    return (i != speed_levels.end()) ? i->second : 0;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡ�ϴ�/�����ٶȼ������Ӧ���ٶ�
 * ��  ��: [in] ifs ��ȡ���ݵ��ļ�
 * ����ֵ: ��ȡ���ٶȼ������Ӧ���ٶ�
 * ��  ��:
 *   ʱ�� 2013.11.02
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static SpeedLevels GetSpeedLimitLevel(std::ifstream& ifs)
{
    std::string line;  // ��ȡ�ļ��е�һ��
    SpeedLevels speed_levels;  // �ٶȵȼ�
    boost::smatch match;  // ������ʽƥ��Ľ��
    boost::regex reg("\\s*(\\w+)\\s*=\\s*(\\w+)\\s*");  // ƥ�� key = value

    while (!ifs.eof())  // �����ļ���β
    {
        // ��ȡһ���ַ������ع������ж��Ƿ�����һ��������Ŀ�ʼ
        auto ch = ifs.get();
        ifs.unget();
        if (ch == '[')
            break;

        std::getline(ifs, line);  // ���ļ��ж�ȡһ��
        if (boost::regex_match(line, match, reg) && match.size() == 3)  // ������ʽƥ��
        {
            speed_levels.insert(std::make_pair(match[1],  // �ٶȼ���
                boost::lexical_cast<SpeedLimit>(match[2])));  // �����Ӧ���ٶ�
        }
    }
 
    return speed_levels;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡip-filter��Ϣ
 * ��  ��: [in] ifs ��ȡ���ݵ��ļ�
 *         [in] speed_levels �ϴ�/�����ٶȼ������Ӧ���ٶ�
 *         [out] filter ��Ŷ�ȡ��ip-filter��Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.02
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
static void ReadIpFilter(std::ifstream& ifs, 
        const SpeedLevels& speed_levels,
        Detail::FilterImpl<ipv4_address::bytes_type, AccessInfo>& filter)
{
    std::string line;  // ��ȡ�ļ��е�һ��
    boost::smatch match;  // ������ʽƥ��Ľ��
    // ƥ��ip-filter��������ʽ
    boost::regex reg("\\s*"
        "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})"  // group 1, ip address
        "\\s*-\\s*"
        "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})"  // group 2, ip address
        "\\s*,\\s*"
        "(\\d+)"  // group 3, access level
        "\\s*,\\s*"
        "(\\w+)"  // group 4, upload speed level
        "\\s*,\\s*"
        "(\\w+)"  // group 5, download speed level
        "\\s*,\\s*"
        "(.*)");  // group 6, description

    while (!ifs.eof())  // �����ļ���β
    {
        std::getline(ifs, line);  // ��ȡ�ļ���һ��
        if (boost::regex_match(line, match, reg) && match.size() == 7)  // ƥ��if-filter������ʽ
        {
            AccessInfo access_info;  // ip-filter��Ӧ�ķ���Ȩ����Ϣ
            access_info.flags = ToAccessFlag(boost::lexical_cast<int>(match[3])),  // ����Ȩ��
            access_info.upload_speed_limit = GetSpeedLimit(speed_levels, match[4]),  // ����ϴ��ٶ�
            access_info.download_speed_limit = GetSpeedLimit(speed_levels, match[5]),  // ��������ٶ�
            access_info.description = match[6];  // ip-filter��Ӧ��������Ϣ

            filter.AddRule(ipv4_address::from_string(match[1]).to_bytes(),  // ip-filter��ʼip��ַ
                ipv4_address::from_string(match[2]).to_bytes(),  // ip-filter�յ�ip��ַ
                access_info);
        } 
    }
}

void IpFilter::AddRule(ip_address first, ip_address last, const AccessInfo& access_info)
{
	if (first.is_v4())
	{
		BC_ASSERT(last.is_v4());
		filter_.AddRule(first.to_v4().to_bytes(), last.to_v4().to_bytes(), access_info);
	}
	else
	{
		BC_ASSERT(false);
	}
}

AccessInfo IpFilter::Access(ip_address const& addr) const
{
    return addr.is_v4() ? filter_.Access(addr.to_v4().to_bytes()) : AccessInfo();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡIpFilter��������
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.02
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
IpFilter& IpFilter::GetInstance()
{
    static IpFilter ip_filter;

    return ip_filter;
}

/*------------------------------------------------------------------------------
 * ��  ��: ���ļ��ж�ȡip-filter��Ϣ
 * ��  ��: [in] file ��ȡ���ݵ��ļ���Ӧ���ļ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.11.02
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
bool IpFilter::Init(const std::string& file)
{
    static const char* const kIpFilterTag = "[ip-filter]";  // ip-filter��Ӧ������
    static const char* const kSpeedLimitLevelTag = "[speed-limit-level]";  // �ٶȼ����Ӧ������

    std::string line;  // �����ļ��е�һ������
    std::ifstream ifs(file);  //��ȡ���ݵ��ļ� 
    SpeedLevels speed_levels;  // �ٶȼ���

    if (!ifs.is_open())
    {
        LOG(INFO) << "Fail to open ip filter file : " << file;
        return false;
    }

    while (!ifs.eof())  // �����ļ���β
    {
        std::getline(ifs, line);  // ��ȡ�ļ���һ��
        if (line == kSpeedLimitLevelTag)  // �ٶȼ����Ӧ������
        {
            speed_levels = GetSpeedLimitLevel(ifs);  // ��ȡ�ٶȼ����Ӧ������
        }
        else if (line == kIpFilterTag)  // ip-filter��Ӧ������
        {
            ReadIpFilter(ifs, speed_levels, filter_);  // ��ȡip-filter��Ӧ������
        }
    }

    return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��:ipfilter����ģ����ض��ļ������
 * 		<first-ip> - <last-ip> , <access> , <comment>
 * 		first-ip �������ַ��Χ�Ŀ�ʼ
 * 	     last-ip  �������ַ��Χ�Ľ���
 * 		acess    �������ַ��Χ��׼�����ͣ� ���� block, inner, outer
 *		comment  ��ע�ͣ� ��������
 * ��  ? std::string const& file   ��ȡ��ipfilter�ļ�
 * ����ֵ:
 * ��  ��:
 * ʱ�� 2013��09��13��
 * ���� tom_liu
 * ���� ����
 ----------------------------------------------------------------------------*/
/*
void IpFilter::LoadIpFilter(std::string const& file)
{
  if (file.empty()) return;
  fs::ifstream in(file);
  if (in.fail()) return;

  boost::regex reg("\\s*(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s*-\\s*(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s*,\\s*(\\d+)\\s*.*");
  boost::regex ip_reg("0*(\\d*)\\.0*(\\d*)\\.0*(\\d*)\\.0*(\\d*)");
  boost::smatch swhat;

  while (in.good())
  {
    std::string line;
    std::getline(in, line);
    if (line.empty()) continue;
    if (line[0] == '#') continue;

    if (boost::regex_match(line, swhat, reg))
    {
      std::string first(swhat[1]);
      std::string last(swhat[2]);
      int flags = boost::lexical_cast<int>(swhat[3]);
      if (flags < 100) 
	  {
	  	flags = BLOCKED;
      }
      else if (flags >= 200 )
	  {
	  	flags =INNER;
      }
      else
	  {
	  	flags = OUTER;
      }

      if (boost::regex_match(first, swhat, ip_reg))
      {
        first = ((swhat.length(1) != 0) ? swhat[1] : std::string("0")) + "." +
          ((swhat.length(2) != 0) ? swhat[2] : std::string("0")) + "." +
          ((swhat.length(3) != 0) ? swhat[3] : std::string("0")) + "." +
          ((swhat.length(4) != 0) ? swhat[4] : std::string("0"));
      }
      if (boost::regex_match(last, swhat, ip_reg))
      {
        last = ((swhat.length(1) != 0) ? swhat[1] : std::string("0")) + "." +
          ((swhat.length(2) != 0) ? swhat[2] : std::string("0")) + "." +
          ((swhat.length(3) != 0) ? swhat[3] : std::string("0")) + "." +
          ((swhat.length(4) != 0) ? swhat[4] : std::string("0"));
      }

      try
      {
         AddRule(ipv4_address::from_string(first),
         	ipv4_address::from_string(last), flags);
      }
      catch(...)
      {
        LOG(FATAL) << "Invalid IP range: " << first << "-" << last << std::endl;
      }
    }
  }
}
*/

void PortFilter::AddRule(boost::uint16_t first, boost::uint16_t last, int flags)
{
	portfilter_.AddRule(first, last, flags);
}

int PortFilter::Access(boost::uint16_t port) const
{
	return portfilter_.Access(port);
}

}

