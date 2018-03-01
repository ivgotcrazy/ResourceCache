/*#############################################################################
 * 文件名   : monitor_manager.hpp.hpp
 * 创建人   : tom_liu
 * 创建时间 : 2013年10月24日
 * 文件描述 : SystemResourceMonitor类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEADER_MONITOR_MANAGER
#define HEADER_MONITOR_MANAGER

#include "bc_typedef.hpp"
#include "basic_monitor.hpp"
#include "bc_util.hpp"
#include "timer.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * 描  述: Cpu查询信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct CpuInfo
{	
	char cpu_name[16]; //Cpu名
	uint32 use_rate;    //使用率
};

/*-----------------------------------------------------------------------------
 * 描  述: Mem查询信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct MemInfo
{
	uint64 total_size; //总的内存大小,单位为G
	uint32 use_rate;
};

/*-----------------------------------------------------------------------------
 * 描  述: Disk查询信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct DiskInfo
{
	char disk_name[32]; //硬盘名
	uint32 total_size; //单位为G
	uint32 use_rate; //使用率
};

/*-----------------------------------------------------------------------------
 * 描  述: Net查询信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct NetInfo
{
    char net_name[16];
	uint64 tx_total_size; //出口流量 ，单位为byte
	uint64 rx_total_size; //入口流量，单位为byte
	uint64 tx_moment_size; //瞬时出口流量，单位为byte
	uint64 rx_moment_size; //瞬时入口流量，单位为byte
};

class SessionManager;

/******************************************************************************
 * 描述: Cpu信息查询和监控
 * 作者：tom_liu
 * 时间：2013/11/09
 *****************************************************************************/
class CpuMonitor
{
public:
	CpuMonitor(SessionManager& session_man);

	void Init();
	
	void GetInfo(std::vector<CpuInfo>& info);
	
	void Monitor();
	
	void AdjustResouce(bool block);

private:
	SessionManager& session_manager_;
	
	bool cpu_adjust_flag_;	
	ptime cpu_adjust_time_; //上一次Cpu调度的时间

	//计算每个Cpu的平均使用率队列容器
	typedef std::deque<uint32> CpuRateQueue;
	CpuRateQueue rate_queue_;
	
	std::vector<CpuStats> priv_stats_vec_;
};

/******************************************************************************
 * 描述: Mem信息查询和监控
 * 作者：tom_liu
 * 时间：2013/11/09
 *****************************************************************************/
class MemMonitor
{
public:
	MemMonitor(SessionManager& session_man);
	
	void Init();
	
	void GetInfo(MemInfo& info);
	
	void Monitor();
	
	void AdjustResouce(bool block);
	
private:
	SessionManager& session_manager_;
	
	bool mem_adjust_flag_;
	ptime mem_adjust_time_; //上一次Mem调度的时间

	//计算每个Mem的平均使用率的队列
	std::deque<uint32> rate_queue_;
};

/******************************************************************************
 * 描述: Disk信息查询和监控
 * 作者：tom_liu
 * 时间：2013/11/09
 *****************************************************************************/
class DiskMonitor
{
public:
	DiskMonitor(SessionManager& session_man);
	
	void Init();
	
	void GetInfo(std::vector<DiskInfo>& info_vec);
	
	void Monitor();
	
	void AdjustResouce(const std::vector<DiskInfo>& adjust_disk_vec);
	
private:
	inline uint64 GalcDeleteSize(const DiskInfo& info);
	
	bool GetDiskAdjustFlag(std::string disk_path_name);

	void GetOneDiskInfo(const std::string disk_name, DiskInfo& info);
	
	void UpdateAcessSet(const std::string disk_path_name);
	
	void StartAdjustThread(const DiskInfo& info);
	void AdjustOneDisk(const DiskInfo& info);
	
private:
	struct TorrentAcessInfo
	{
		ptime last_access_time;
		std::string hash_str;
		std::string parent_path;
	};

	struct Compare
	{	
		//自定义一个仿函数
		bool operator ()(const TorrentAcessInfo& priv_info, const TorrentAcessInfo& cur_info)
		{
			return priv_info.last_access_time < cur_info.last_access_time;
		}
	};
	
	//有相同访问时间的文件
	typedef std::multiset<TorrentAcessInfo,Compare>  TorrentAcessInfoSet;

	void AddAcessInfo(const std::string& directory, const std::string& parent_path, 
														TorrentAcessInfoSet& info_set);
	void CleanSomeTorrentFile(TorrentAcessInfoSet& info_set, uint64 delete_size);
	
private:
	SessionManager& session_manager_;
	bool adjust_flag_;
	
	std::vector<TorrentAcessInfoSet> access_info_vec_;

	//用于标志硬盘是否存于调度状态
	typedef std::map<std::string, bool> AdjustFlagMap;
	AdjustFlagMap adjust_flag_map_; 
	
	typedef std::map<std::string,TorrentAcessInfoSet>  TorrentAcessInfoMap;
	TorrentAcessInfoMap access_info_map_;
};

/******************************************************************************
 * 描述: Net信息查询和监控
 * 作者：tom_liu
 * 时间：2013/11/09
 *****************************************************************************/
class NetMonitor
{
public:
	NetMonitor(SessionManager& session_man);
	
	void Init();
	
	void GetInfo(std::vector<NetInfo>& info_vec);
	
	void Monitor();
	
	void AdjustResouce(bool block);

private:
	SessionManager& session_manager_;
	
	//保存第一次的netstats状态信息，用于后面做统计
	std::vector<NetStats> first_stats_vec_;
	std::vector<NetStats> priv_stats_vec_;
};

/******************************************************************************
 * 描述: 信息查询和监控总调度
 * 作者：tom_liu
 * 时间：2013/11/09
 *****************************************************************************/
class MonitorManager
{
public:
	MonitorManager(SessionManager& ses);
	
	void Run();

	//信息获取相关
	void GetCpuInfo(std::vector<CpuInfo>& info_vec);
	void GetMemInfo(MemInfo& info);
	void GetDiskInfo(std::vector<DiskInfo>& info_vec);
	void GetNetInfo(std::vector<NetInfo>& info_vec);
	
private:
	void Init();

	void IoServiceThread(io_service& ios, std::string thread_name);

	void Monitor();
	
private:
	SessionManager& session_manager_;

	bool stop_flag_;
	
	CpuMonitor cpu_monitor_;
	MemMonitor mem_monitor_;
	DiskMonitor disk_monitor_;
	NetMonitor net_monitor_; 

	boost::asio::io_service monitor_ios_;
	Timer timer_;
	
	boost::thread monitor_thread_;
};

}
#endif

