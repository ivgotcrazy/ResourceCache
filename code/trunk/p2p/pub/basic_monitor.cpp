/*#############################################################################
 * �ļ���   : basic_monitor.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��11��08��
 * �ļ����� : ϵͳ��Դ�ײ���Ϣץȡʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
 
#include "basic_monitor.hpp"

#include <mntent.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>


#define CPU_STAT_FILE "/proc/stat"
#define MEM_STAT_FILE "/proc/meminfo"
#define DISK_STAT_FILE "/etc/mtab"
#define NET_STAT_FILE "/proc/net/dev"

#define SMALL_LINE_LEN 128
#define BIG_LINE_LEN 4096

namespace BroadCache
{

int ReadPartitionStat(const char *fsname, DiskStats *sp)
{
    struct statfs fsbuf;
    if (!statfs(fsname, &fsbuf)) {
		strcpy(sp->disk_name, fsname);
        sp->block_size = fsbuf.f_bsize;
        sp->total_blocks = fsbuf.f_blocks;
        sp->free_blocks = fsbuf.f_bfree;
        sp->avail_blocks = fsbuf.f_bavail;
    }
    return 0;
}
uint64 CountJiffies(const CpuStats stat)
{
	uint64  total_jiffies ;
	total_jiffies = stat.cpu_user +  stat.cpu_sys + stat.cpu_nice + stat.cpu_idle + stat.cpu_iowait
		+ stat.cpu_hardirq + stat.cpu_softirq + stat.cpu_steal + stat.cpu_guest;
	return total_jiffies;
}


void UpdateCpuStats(CpuStats& stats, const CpuStats new_stats)
{
	stats.cpu_user = new_stats.cpu_user;
	stats.cpu_nice = new_stats.cpu_nice;
	stats.cpu_sys = new_stats.cpu_sys;
	stats.cpu_idle = new_stats.cpu_idle;
	stats.cpu_iowait = new_stats.cpu_iowait;
	stats.cpu_steal = new_stats.cpu_steal;
	stats.cpu_hardirq = new_stats.cpu_hardirq;
	stats.cpu_softirq = new_stats.cpu_softirq;
	stats.cpu_guest = new_stats.cpu_guest;
}

void UpdateNetStats(NetStats& stats, const NetStats new_stats)
{
	stats.in_bytes = new_stats.in_bytes;
	stats.out_bytes = new_stats.out_bytes;
	stats.in_pkt = new_stats.in_pkt;
	stats.out_pkt = new_stats.out_pkt;
}


/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡϵͳCpu��Դʹ������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetCpuStats(std::vector<CpuStats>& stats_vec)
{
	
	char line[BIG_LINE_LEN];
	char cpuname[16];
	FILE *fp;
	CpuStats temp;
	
	memset(&temp, 0, sizeof(struct CpuStats));
	if ((fp = fopen(CPU_STAT_FILE, "r")) == NULL) 
	{
		return ;
	}
	
	while (fgets(line, BIG_LINE_LEN, fp) != NULL) 
	{
		if (!strncmp(line, "cpu", 3)) 
		{
			sscanf(line, "%4s", cpuname);
			
			strcpy(temp.cpu_name,cpuname);
			sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu %llu %llu",
					&temp.cpu_user,
					&temp.cpu_nice,
					&temp.cpu_sys,
					&temp.cpu_idle,
					&temp.cpu_iowait,
					&temp.cpu_hardirq,
					&temp.cpu_softirq,
					&temp.cpu_steal,
					&temp.cpu_guest);
			
			stats_vec.push_back(temp);
		}
	}
	fclose(fp);
}



/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡϵͳMem��Դʹ������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetMemStats(MemStats& stat)
{
	FILE *fp;
	char line[SMALL_LINE_LEN];
	char buf[BIG_LINE_LEN];

	memset(buf, 0, BIG_LINE_LEN);
	memset(&stat, 0, sizeof(MemStats));
	if ((fp = fopen(MEM_STAT_FILE, "r")) == NULL) 
	{
		return ;
	}

	while (fgets(line, 128, fp) != NULL)
	{
		if (!strncmp(line, "MemTotal:", 9)) 
		{
			//��ȡ�ܵ��ڴ��С
			sscanf(line + 9, "%lu", &stat.total_kb);
		}
		else if (!strncmp(line, "MemFree:", 8)) 
		{
			//��ȡ���е��ڴ��С
			sscanf(line + 8, "%lu", &stat.free_kb);
		}
		else if (!strncmp(line, "Buffers:", 8)) 
		{
			//��ȡ���ڻ���дӲ�̲����Ļ����С
			sscanf(line + 8, "%lu", &stat.buffer_kb);
		}
		else if (!strncmp(line, "Cached:", 7)) 
		{
			//��ȡ���ڻ����Ӳ�̲����Ļ����С
			sscanf(line + 7, "%lu", &stat.cache_kb);
		}
	}
	
	fclose(fp);
}



/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡϵͳDisk��Դʹ������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetDiskStats(std::vector<DiskStats>& stats_vec)
{
	char buf[BIG_LINE_LEN];
    FILE *mntfile;
    struct mntent *mnt = NULL;
    DiskStats  temp;

    memset(buf, 0, BIG_LINE_LEN);
    memset(&temp, 0, sizeof(temp));
    mntfile = setmntent(DISK_STAT_FILE, "r");

    //�������ر��
    while ((mnt = getmntent(mntfile)) != NULL) 
	{
        //ֻƥ��disk��ͷ��
        if (!strncmp(mnt->mnt_dir, "/disk", 5)) 
		{
            //��ȡ������Ϣ
            ReadPartitionStat(mnt->mnt_dir, &temp);
			stats_vec.push_back(temp);
		}
		memset(&temp, 0, sizeof(temp));
    }
    endmntent(mntfile);
}



/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡϵͳNet��Դʹ������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetNetStats(std::vector<NetStats>& stats_vec)
{
    FILE *fp;
    char *p = NULL;
    char line[BIG_LINE_LEN] = {0};
    char buf[BIG_LINE_LEN] = {0};
	NetStats temp;

    memset(buf, 0, BIG_LINE_LEN);
    memset(&temp, 0, sizeof(NetStats));

    if ((fp = fopen(NET_STAT_FILE, "r")) == NULL) 
	{
        return;
    }

    while (fgets(line, BIG_LINE_LEN, fp) != NULL) 
	{
        if (strstr(line, "eth") || strstr(line, "em")) 
		{
            memset(&temp, 0, sizeof(temp));
            p = strchr(line, ':');

			//���� net_name
			strncpy(temp.net_name, line, p-line);
            sscanf(p + 1, "%llu %llu %*u %*u %*u %*u %*u %*u "
                    "%llu %llu %*u %*u %*u %*u %*u %*u",
                    &temp.in_bytes,
                    &temp.in_pkt,
                    &temp.out_bytes,
                    &temp.out_pkt);

			//LOG(INFO) << "In:" << temp.in_bytes << " Out:" << temp.out_bytes;
			
			stats_vec.push_back(temp);
        }
    }

    fclose(fp);
}

}





