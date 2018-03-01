#include "ip_filter.hpp"
#include <boost/utility.hpp>

namespace fs = boost::filesystem;	

namespace BroadCache
{

typedef std::string SpeedLevel;  // 上传，下载速度级别
typedef unsigned int SpeedLimit;  // 上传，下载速度极限
typedef std::map<SpeedLevel, SpeedLimit> SpeedLevels;  // 上传，下载速度级别对应速度数值

/*------------------------------------------------------------------------------
 * 描  述: 将访问限制数值转换成访问权限
 * 参  数: [in] access_level 访问限制数值
 * 返回值: 访问权限
 * 修  改:
 *   时间 2013.11.02
 *   作者 rosan
 *   描述 创建
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
 * 描  述: 获取某一个级别的上传/下载速度
 * 参  数: [in] speed_levels 每一个级别对应的速度字典
 *         [in] level 速度级别  
 * 返回值: 此级别对应的速度
 * 修  改:
 *   时间 2013.11.02
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static SpeedLimit GetSpeedLimit(const SpeedLevels& speed_levels, const SpeedLevel& level)
{ 
    auto i = speed_levels.find(level);
    return (i != speed_levels.end()) ? i->second : 0;
}

/*------------------------------------------------------------------------------
 * 描  述: 从文件中读取上传/下载速度级别及其对应的速度
 * 参  数: [in] ifs 读取数据的文件
 * 返回值: 读取的速度级别及其对应的速度
 * 修  改:
 *   时间 2013.11.02
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static SpeedLevels GetSpeedLimitLevel(std::ifstream& ifs)
{
    std::string line;  // 读取文件中的一行
    SpeedLevels speed_levels;  // 速度等级
    boost::smatch match;  // 正则表达式匹配的结果
    boost::regex reg("\\s*(\\w+)\\s*=\\s*(\\w+)\\s*");  // 匹配 key = value

    while (!ifs.eof())  // 不是文件结尾
    {
        // 读取一个字符，并回滚，以判断是否是下一个配置项的开始
        auto ch = ifs.get();
        ifs.unget();
        if (ch == '[')
            break;

        std::getline(ifs, line);  // 从文件中读取一行
        if (boost::regex_match(line, match, reg) && match.size() == 3)  // 正则表达式匹配
        {
            speed_levels.insert(std::make_pair(match[1],  // 速度级别
                boost::lexical_cast<SpeedLimit>(match[2])));  // 级别对应的速度
        }
    }
 
    return speed_levels;
}

/*------------------------------------------------------------------------------
 * 描  述: 从文件中读取ip-filter信息
 * 参  数: [in] ifs 读取数据的文件
 *         [in] speed_levels 上传/下载速度级别及其对应的速度
 *         [out] filter 存放读取的ip-filter信息
 * 返回值:
 * 修  改:
 *   时间 2013.11.02
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
static void ReadIpFilter(std::ifstream& ifs, 
        const SpeedLevels& speed_levels,
        Detail::FilterImpl<ipv4_address::bytes_type, AccessInfo>& filter)
{
    std::string line;  // 读取文件中的一行
    boost::smatch match;  // 正则表达式匹配的结果
    // 匹配ip-filter的正则表达式
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

    while (!ifs.eof())  // 不是文件结尾
    {
        std::getline(ifs, line);  // 获取文件的一行
        if (boost::regex_match(line, match, reg) && match.size() == 7)  // 匹配if-filter正则表达式
        {
            AccessInfo access_info;  // ip-filter对应的访问权限信息
            access_info.flags = ToAccessFlag(boost::lexical_cast<int>(match[3])),  // 访问权限
            access_info.upload_speed_limit = GetSpeedLimit(speed_levels, match[4]),  // 最大上传速度
            access_info.download_speed_limit = GetSpeedLimit(speed_levels, match[5]),  // 最大下载速度
            access_info.description = match[6];  // ip-filter对应的描述信息

            filter.AddRule(ipv4_address::from_string(match[1]).to_bytes(),  // ip-filter起始ip地址
                ipv4_address::from_string(match[2]).to_bytes(),  // ip-filter终点ip地址
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
 * 描  述: 获取IpFilter单例对象
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013.11.02
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
IpFilter& IpFilter::GetInstance()
{
    static IpFilter ip_filter;

    return ip_filter;
}

/*------------------------------------------------------------------------------
 * 描  述: 从文件中读取ip-filter信息
 * 参  数: [in] file 读取数据的文件对应的文件名
 * 返回值:
 * 修  改:
 *   时间 2013.11.02
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
bool IpFilter::Init(const std::string& file)
{
    static const char* const kIpFilterTag = "[ip-filter]";  // ip-filter对应的配置
    static const char* const kSpeedLimitLevelTag = "[speed-limit-level]";  // 速度级别对应的配置

    std::string line;  // 保存文件中的一行数据
    std::ifstream ifs(file);  //读取数据的文件 
    SpeedLevels speed_levels;  // 速度级别

    if (!ifs.is_open())
    {
        LOG(INFO) << "Fail to open ip filter file : " << file;
        return false;
    }

    while (!ifs.eof())  // 不是文件结尾
    {
        std::getline(ifs, line);  // 读取文件的一行
        if (line == kSpeedLimitLevelTag)  // 速度级别对应的配置
        {
            speed_levels = GetSpeedLimitLevel(ifs);  // 读取速度级别对应的配置
        }
        else if (line == kIpFilterTag)  // ip-filter对应的配置
        {
            ReadIpFilter(ifs, speed_levels, filter_);  // 读取ip-filter对应的配置
        }
    }

    return true;
}

/*-----------------------------------------------------------------------------
 * 描  述:ipfilter解析模块和特定文件相关联
 * 		<first-ip> - <last-ip> , <access> , <comment>
 * 		first-ip 是这个地址范围的开始
 * 	     last-ip  是这个地址范围的结束
 * 		acess    是这个地址范围的准入类型， 包括 block, inner, outer
 *		comment  是注释， 不做解析
 * 参  ? std::string const& file   读取的ipfilter文件
 * 返回值:
 * 修  改:
 * 时间 2013年09月13日
 * 作者 tom_liu
 * 描述 创建
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

