/*#############################################################################
 * 文件名   : basic_monitor.hpp
 * 创建人   : tom_liu
 * 创建时间 : 2013年11月08日
 * 文件描述 : 底层数据监控获取文件
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEADER_BASIC_MONITOR
#define HEADER_BASIC_MONITOR

#include "depend.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * 描  述: Cpu信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct CpuStats 
{	
	char cpu_name[16];
    unsigned long long cpu_user;
    unsigned long long cpu_nice;
    unsigned long long cpu_sys;
    unsigned long long cpu_idle;
    unsigned long long cpu_iowait;
    unsigned long long cpu_steal;
    unsigned long long cpu_hardirq;
    unsigned long long cpu_softirq;
    unsigned long long cpu_guest;
};

/*-----------------------------------------------------------------------------
 * 描  述: 内存信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct MemStats 
{
	unsigned long total_kb;
	unsigned long free_kb;
	unsigned long buffer_kb;
	unsigned long cache_kb;
};

/*-----------------------------------------------------------------------------
 * 描  述: 分区(Partion)信息也即Disk信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct DiskStats 
{
	char disk_name[32];	
    int block_size;	//一个block大小，通常为4096
    unsigned long long total_blocks; //总共的block， 硬盘容量为 total_blocks * block_size
    unsigned long long free_blocks; //空闲的block ， 做统计时已用block = total_blocks - free_blocks
    unsigned long long avail_blocks; //非root用户能用的block
};

/*-----------------------------------------------------------------------------
 * 描  述: 网络信息
 * 作  者: tom_liu
 * 时  间: 2013年10月30日
 ----------------------------------------------------------------------------*/ 
struct NetStats 
{
	char net_name[32];
    unsigned long long in_bytes; 
    unsigned long long out_bytes;
    unsigned long long in_pkt;
    unsigned long long out_pkt;
};

int ReadPartitionStat(const char *fsname, DiskStats *sp);

uint64 CountJiffies(const CpuStats stat);

void UpdateCpuStats(CpuStats& stats, const CpuStats new_stats);
void UpdateNetStats(NetStats& stats, const NetStats new_stats);
	
void GetCpuStats(std::vector<CpuStats>& stats_vec);
void GetMemStats(MemStats& info);
void GetDiskStats(std::vector<DiskStats>& stats_vec);
void GetNetStats(std::vector<NetStats>& stats_vec);

}

#endif


