/*#############################################################################
 * �ļ���   : basic_monitor.hpp
 * ������   : tom_liu
 * ����ʱ�� : 2013��11��08��
 * �ļ����� : �ײ����ݼ�ػ�ȡ�ļ�
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef HEADER_BASIC_MONITOR
#define HEADER_BASIC_MONITOR

#include "depend.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * ��  ��: Cpu��Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
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
 * ��  ��: �ڴ���Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
 ----------------------------------------------------------------------------*/ 
struct MemStats 
{
	unsigned long total_kb;
	unsigned long free_kb;
	unsigned long buffer_kb;
	unsigned long cache_kb;
};

/*-----------------------------------------------------------------------------
 * ��  ��: ����(Partion)��ϢҲ��Disk��Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
 ----------------------------------------------------------------------------*/ 
struct DiskStats 
{
	char disk_name[32];	
    int block_size;	//һ��block��С��ͨ��Ϊ4096
    unsigned long long total_blocks; //�ܹ���block�� Ӳ������Ϊ total_blocks * block_size
    unsigned long long free_blocks; //���е�block �� ��ͳ��ʱ����block = total_blocks - free_blocks
    unsigned long long avail_blocks; //��root�û����õ�block
};

/*-----------------------------------------------------------------------------
 * ��  ��: ������Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
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


