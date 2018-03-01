/*#############################################################################
 * �ļ���   : monitor_manager.hpp.hpp
 * ������   : tom_liu
 * ����ʱ�� : 2013��10��24��
 * �ļ����� : SystemResourceMonitor������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: Cpu��ѯ��Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
 ----------------------------------------------------------------------------*/ 
struct CpuInfo
{	
	char cpu_name[16]; //Cpu��
	uint32 use_rate;    //ʹ����
};

/*-----------------------------------------------------------------------------
 * ��  ��: Mem��ѯ��Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
 ----------------------------------------------------------------------------*/ 
struct MemInfo
{
	uint64 total_size; //�ܵ��ڴ��С,��λΪG
	uint32 use_rate;
};

/*-----------------------------------------------------------------------------
 * ��  ��: Disk��ѯ��Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
 ----------------------------------------------------------------------------*/ 
struct DiskInfo
{
	char disk_name[32]; //Ӳ����
	uint32 total_size; //��λΪG
	uint32 use_rate; //ʹ����
};

/*-----------------------------------------------------------------------------
 * ��  ��: Net��ѯ��Ϣ
 * ��  ��: tom_liu
 * ʱ  ��: 2013��10��30��
 ----------------------------------------------------------------------------*/ 
struct NetInfo
{
    char net_name[16];
	uint64 tx_total_size; //�������� ����λΪbyte
	uint64 rx_total_size; //�����������λΪbyte
	uint64 tx_moment_size; //˲ʱ������������λΪbyte
	uint64 rx_moment_size; //˲ʱ�����������λΪbyte
};

class SessionManager;

/******************************************************************************
 * ����: Cpu��Ϣ��ѯ�ͼ��
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/09
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
	ptime cpu_adjust_time_; //��һ��Cpu���ȵ�ʱ��

	//����ÿ��Cpu��ƽ��ʹ���ʶ�������
	typedef std::deque<uint32> CpuRateQueue;
	CpuRateQueue rate_queue_;
	
	std::vector<CpuStats> priv_stats_vec_;
};

/******************************************************************************
 * ����: Mem��Ϣ��ѯ�ͼ��
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/09
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
	ptime mem_adjust_time_; //��һ��Mem���ȵ�ʱ��

	//����ÿ��Mem��ƽ��ʹ���ʵĶ���
	std::deque<uint32> rate_queue_;
};

/******************************************************************************
 * ����: Disk��Ϣ��ѯ�ͼ��
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/09
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
		//�Զ���һ���º���
		bool operator ()(const TorrentAcessInfo& priv_info, const TorrentAcessInfo& cur_info)
		{
			return priv_info.last_access_time < cur_info.last_access_time;
		}
	};
	
	//����ͬ����ʱ����ļ�
	typedef std::multiset<TorrentAcessInfo,Compare>  TorrentAcessInfoSet;

	void AddAcessInfo(const std::string& directory, const std::string& parent_path, 
														TorrentAcessInfoSet& info_set);
	void CleanSomeTorrentFile(TorrentAcessInfoSet& info_set, uint64 delete_size);
	
private:
	SessionManager& session_manager_;
	bool adjust_flag_;
	
	std::vector<TorrentAcessInfoSet> access_info_vec_;

	//���ڱ�־Ӳ���Ƿ���ڵ���״̬
	typedef std::map<std::string, bool> AdjustFlagMap;
	AdjustFlagMap adjust_flag_map_; 
	
	typedef std::map<std::string,TorrentAcessInfoSet>  TorrentAcessInfoMap;
	TorrentAcessInfoMap access_info_map_;
};

/******************************************************************************
 * ����: Net��Ϣ��ѯ�ͼ��
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/09
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
	
	//�����һ�ε�netstats״̬��Ϣ�����ں�����ͳ��
	std::vector<NetStats> first_stats_vec_;
	std::vector<NetStats> priv_stats_vec_;
};

/******************************************************************************
 * ����: ��Ϣ��ѯ�ͼ���ܵ���
 * ���ߣ�tom_liu
 * ʱ�䣺2013/11/09
 *****************************************************************************/
class MonitorManager
{
public:
	MonitorManager(SessionManager& ses);
	
	void Run();

	//��Ϣ��ȡ���
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

