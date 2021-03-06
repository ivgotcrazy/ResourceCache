/*#############################################################################
 * 文件名   : monitor_manager.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年10月24日
 * 文件描述 : SystemResourceMonitor类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "monitor_manager.hpp"
#include "basic_monitor.hpp"
#include "session_manager.hpp"

namespace BroadCache
{
const uint32 kCpuOverloadRate = 90;
const uint32 kMemOverloadRate = 95;
const uint32 kDiskOverloadRate = 90; //硬盘过载使用率
const uint32 kDiskExcateRate = 80; //硬盘饱满使用率
const int64 kAdjustExpireTime = 20; //资源调度最短间隔时间
const uint32 kAverageNum = 5; //计算平均使用率，保留在queue中记录的个数
const uint32 kTimerClock = 10; //定时器间隔时间

/*-----------------------------------------------------------------------------
 * 描  述: 计算平均值函数
 * 参  数: rate_queue  保存数值的队列
 * 返回值: rate 最新一次的使用率
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
static uint32 CalcAverage(std::deque<uint32>& rate_queue, uint32 rate)
{
	if (rate_queue.size() > (kAverageNum -1))
	{
		rate_queue.pop_front();
	}
	rate_queue.push_back(rate);

    uint32 max_rate = *(std::max_element(rate_queue.begin(), rate_queue.end()));
	uint32 min_rate = *(std::max_element(rate_queue.begin(), rate_queue.end()));

	int total_rate = 0;
	for (uint32& rate: rate_queue)
	{
		total_rate += rate;
	}

	//保留5次记录，去掉一个最高值，去掉一个最低值,取平均值
	uint32  per_rate = 0;
	if (rate_queue.size() > kAverageNum - 2)
	{	
		per_rate = (total_rate - max_rate -min_rate) / (rate_queue.size() -2);
	}
	else
	{
		per_rate = total_rate / rate_queue.size(); 
	}
	
	return per_rate;
}

//=============================CpuMonitor================================

/*-----------------------------------------------------------------------------
 * 描  述: CpuMonitor构造函数
 * 参  数: [in]ses SessionManager引用
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
CpuMonitor::CpuMonitor(SessionManager& session_man): session_manager_(session_man)
													   , cpu_adjust_flag_(false)
{
}

void CpuMonitor::Init()
{
	LOG(INFO) << "Cpu Monitor Init";
	//获取初始cpu数据
	GetCpuStats(priv_stats_vec_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 查询Cpu使用情况
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void CpuMonitor::GetInfo(std::vector<CpuInfo>& info_vec)
{
	std::vector<CpuStats> now_stats_vec;
	GetCpuStats(now_stats_vec);

	CpuInfo info;
	for (uint32 i=0; i < now_stats_vec.size(); i++)
	{
		strcpy(info.cpu_name, priv_stats_vec_[i].cpu_name);

		//jiffies是内核中的一个全局变量，用来记录自系统启动一来产生的节拍数
		uint64 priv_total_jiffies = CountJiffies(priv_stats_vec_[i]);
		uint64 now_total_jiffies = CountJiffies(now_stats_vec[i]);
		//LOG(INFO) << "priv:" << priv_total_jiffies << "now" << now_total_jiffies;
		uint64 total_jiffies = now_total_jiffies - priv_total_jiffies;
		uint64 idle_total_jiffies = now_stats_vec[i].cpu_idle - priv_stats_vec_[i].cpu_idle;
		uint32 use_rates = 0;
		if (total_jiffies != 0) //可能发生除0错误
		{
			use_rates = 100 - ((idle_total_jiffies * 100) / total_jiffies);
		}
		if (use_rates > 100 ) use_rates = 100;
		
		info.use_rate = use_rates;
		info_vec.push_back(info);

		//查询完后更新 priv_stats_vec的值
		UpdateCpuStats(priv_stats_vec_[i],now_stats_vec[i]);
	}	
}

/*-----------------------------------------------------------------------------
 * 描  述: 调整Cpu资源接口
 * 参  数: block ture表阻塞 false表恢复
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void CpuMonitor::AdjustResouce(bool block)
{
	LOG(INFO) << "Adjust Cpu Resource | block:" << block;
	if(block)
	{
		session_manager_.StopAcceptNewConnection();
	}
	else
	{
		session_manager_.ResumeAcceptNewConnection();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 监测Cpu资源
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void CpuMonitor::Monitor()
{ 
	LOG(INFO) << "Cpu Monitor -----------------";
	//std::cout << "Init Time: " << cpu_adjust_time_  << " Now Time: " << TimeNow();
	
	//如果调控时间间距不超过最短时间，直接跳过
	int64 seconds = GetDurationSeconds(cpu_adjust_time_, TimeNow());
	if (seconds < kAdjustExpireTime) return;

	std::vector<CpuInfo> info_vec;
    GetInfo(info_vec);
	if (info_vec.empty()) return;
	uint32 real_rate = CalcAverage(rate_queue_, info_vec[0].use_rate); //只需取整体cpu信息
	if (real_rate >= kCpuOverloadRate)
	{
		cpu_adjust_time_ = TimeNow();
		cpu_adjust_flag_ = true;
		//AdjustResouce(true);
	}
	else  //使用率正常后， 重置session的相关标识
	{
		cpu_adjust_flag_ = false;
		//AdjustResouce(false);
	}
	
}

//=============================MemMonitor================================

/*-----------------------------------------------------------------------------
 * 描  述: MemMonitor构造函数
 * 参  数: [in]ses SessionManager引用
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
MemMonitor::MemMonitor(SessionManager& ses): session_manager_(ses) ,mem_adjust_flag_(false)
{
}


/*-----------------------------------------------------------------------------
 * 描  述: 查询Mem使用情况
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MemMonitor::GetInfo(MemInfo& info)
{
	MemStats mem_st;
	GetMemStats(mem_st);
	uint32 use_rate = ((mem_st.total_kb - mem_st.free_kb) * 100) / mem_st.total_kb;

	info.total_size = mem_st.total_kb / (1024 * 1024);
	info.use_rate = use_rate;
}

/*-----------------------------------------------------------------------------
 * 描  述: 调整Mem资源接口
 * 参  数: block ture表阻塞 false表恢复
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MemMonitor::AdjustResouce(bool block)
{
	LOG(INFO) << "Adjust Mem Resource | block:" << block;
	if(block)
	{
		session_manager_.StopAcceptNewConnection();
	}
	else
	{
		session_manager_.ResumeAcceptNewConnection();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 监测Mem资源
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MemMonitor::Monitor()
{ 
	MemInfo meminfo;
	uint32 real_rate;
	GetInfo(meminfo);
	
	real_rate = CalcAverage(rate_queue_, meminfo.use_rate);

	//如果调控时间间距不超过最短时间，直接跳过
	int64 seconds = GetDurationSeconds(mem_adjust_time_, TimeNow());
	if (seconds < kAdjustExpireTime) return;
	
	if (real_rate > kMemOverloadRate)
	{
		mem_adjust_time_ = TimeNow();
		mem_adjust_flag_ = true;
		AdjustResouce(true);
	}
	else if (mem_adjust_flag_) //使用率正常后， 重置session的相关标识
	{
		mem_adjust_flag_ = false;
		AdjustResouce(false);
	}
}

//=============================DiskMonitor================================

/*-----------------------------------------------------------------------------
 * 描  述: DiskMonitor构造函数
 * 参  数: ses SessionManager引用
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
DiskMonitor::DiskMonitor(SessionManager& ses): session_manager_(ses)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::Init()
{
	//初始化 adjust_flag_map_ 
	std::vector<DiskStats> disk_vec;
	GetDiskStats(disk_vec);

	for (auto stats : disk_vec)
	{
		std::string disk_name(stats.disk_name);
		bool flag =false;
		adjust_flag_map_.insert(std::make_pair(disk_name, flag));
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 查询Disk使用情况
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::GetInfo(std::vector<DiskInfo>& info_vec)
{
    std::vector<DiskStats> disk_vec;
	GetDiskStats(disk_vec);

	//做DiskInfo的统计信息处理
	DiskInfo temp_info;
	for (auto stats : disk_vec)
	{
		strcpy(temp_info.disk_name, stats.disk_name);
		temp_info.total_size = (stats.total_blocks * stats.block_size) / (1024 * 1024 * 1024); 
		
		//使用率计算, 用 free_blocks 与 df 命令一致
		temp_info.use_rate = 100 - (stats.free_blocks * 100 / stats.total_blocks);
		
		//LOG(INFO) << "use  size:" << (stats.total_blocks-stats.free_blocks)*4/1024; 
		info_vec.push_back(temp_info);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 根据磁盘名获取一个磁盘的信息
 * 参  数: [in] disk_name 磁盘名 
 		 [out] info  Disk信息
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::GetOneDiskInfo(const std::string disk_name, DiskInfo& info)
{
    std::vector<DiskStats> disk_vec;
	GetDiskStats(disk_vec);

	for (auto stats : disk_vec)
	{
		if (strcmp(disk_name.c_str(), stats.disk_name) == 0)
		{
			strcpy(info.disk_name, stats.disk_name);
			info.total_size = (stats.total_blocks * stats.block_size) / (1024 * 1024 * 1024); 
			info.use_rate = 100 - (stats.free_blocks * 100 / stats.total_blocks);
			LOG(INFO) << "Get One Disk Info | disk_name: " << disk_name;
			break;
		}
	}
}


/*-----------------------------------------------------------------------------
 * 描  述: 更新文件访问集合
 * 参  数: [in] disk_path_name 硬盘名
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::UpdateAcessSet(const std::string disk_path_name)
{
	//计算最新的access_info_map_
	TorrentAcessInfoMap::iterator it;
	it = access_info_map_.find(disk_path_name);
	if (it != access_info_map_.end() &&  !it->second.empty())
	{
		return ; //TorrentAcessInfoSet中有值就直接退出
	}

	std::vector<std::string> out_path_vec;
	session_manager_.CalcShouldDeletePath(disk_path_name, out_path_vec);

	error_code ec;
	if (it->second.empty())  // 更新 set中的数据
	{
		for (std::string& path : out_path_vec)
		{
			fs::path current_path(path);
			if (fs::exists(current_path, ec))
			{
			   fs::directory_iterator item(current_path);
			   fs::directory_iterator item_end;
			   for ( ; item != item_end; item++)
			   {
			   		if (!fs::is_directory(item->path())) continue; //不是目录就不是保存torrent的目录
					LOG(INFO) << "Update acess info | path:" << item->path().string();
					if (session_manager_.IsPathInUse(item->path().leaf().string()))  continue;
					AddAcessInfo(item->path().leaf().string(), path, it->second);
			   }
			}
		}		
	}
	else  //新增set
	{
		for (std::string& path : out_path_vec)
		{
			TorrentAcessInfoSet  acess_set;
			fs::path current_path(path);
			if (fs::exists(current_path, ec))
			{
			   fs::directory_iterator item(current_path);
			   fs::directory_iterator item_end;
			   for ( ; item != item_end; item++)
			   {
			   		if (!fs::is_directory(item->path())) continue; //不是目录就不是保存torrent的目录
					LOG(INFO) << "Update acess info | path:" << item->path().string();
					if (session_manager_.IsPathInUse(item->path().leaf().string())) continue;
					AddAcessInfo(item->path().leaf().string(), path, acess_set);
			   }
			}
			access_info_map_.insert(std::make_pair(disk_path_name, acess_set));
		}	
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 新增一个文件访问信息
 * 参  数: [in] directory torrent保存目录 
 		 [in] path  父路径
 		 [out] info_set 保存文件信息的集合
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::AddAcessInfo(const std::string& directory, const std::string& parent_path, 
										TorrentAcessInfoSet	& info_set)
{
	TorrentAcessInfo  access_info;
	access_info.last_access_time = GetFileLastReadTime(directory);
	access_info.hash_str = directory;
	access_info.parent_path = parent_path;
	info_set.insert(access_info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 清理硬盘上的部分Torrent相关文件
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::CleanSomeTorrentFile(TorrentAcessInfoSet& info_set, uint64 delete_size)
{
	uint64 cur_size = 0;
	TorrentAcessInfoSet::iterator cur_iterator = info_set.begin();
	while (cur_size < delete_size && cur_iterator != info_set.end())
	{
		fs::path real_path((*cur_iterator).parent_path + (*cur_iterator).hash_str);
		int directory_size = CalcDirectorySize(real_path);
		cur_size += directory_size;

		error_code ec;
		fs::remove_all(real_path,ec);
		//记录读不能被删的情况
		if (ec && ec.value() != boost::system::errc::device_or_resource_busy)
		{
			LOG(INFO) << "Can't delete directory | reason:" << ec.message();
		}
		
		cur_iterator++;
	}	
	
	info_set.erase(info_set.begin(), cur_iterator);
}	

/*-----------------------------------------------------------------------------
 * 描  述: 开启调控线程用于调度每一块硬盘
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::StartAdjustThread(const DiskInfo& info)
{
	boost::thread t(boost::bind(&DiskMonitor::AdjustOneDisk, this, info));
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取当前硬盘的调度标志
 * 参  数: [in] disk_path_name  硬盘名
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool DiskMonitor::GetDiskAdjustFlag(std::string disk_path_name)
{
	AdjustFlagMap::iterator it;
	it = adjust_flag_map_.find(disk_path_name);
	if (it == adjust_flag_map_.end()) return false;

	return it->second;
}

/*-----------------------------------------------------------------------------
 * 描  述: 计算要删除的磁盘空间大小
 * 参  数: [in] info  硬盘信息
 * 返回值: uint64  空间大小
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
inline uint64 DiskMonitor::GalcDeleteSize(const DiskInfo& info)
{
	uint64 g_unit = (((info.use_rate - kDiskExcateRate) * 100) / 100 * info.total_size + 1) / 100;
	uint64 kb_unit = g_unit * 1024 * 1024;
	return kb_unit;
}



/*-----------------------------------------------------------------------------
 * 描  述: 调整一个Disk资源接口
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::AdjustOneDisk(const DiskInfo& info)
{
	LOG(INFO) << "Start new thread adjust one disk resource";
	std::string disk_path_name(info.disk_name);

	while (GetDiskAdjustFlag(disk_path_name))
	{	
		LOG(INFO) << "Adjust disk loop begin";
		//更新access_info_map_
		UpdateAcessSet(disk_path_name);

		TorrentAcessInfoMap::iterator iter;
		iter = access_info_map_.find(disk_path_name);
		if (iter == access_info_map_.end())
		{
			LOG(ERROR) << "Don't have this Disk";
			break;
		}
		if (iter->second.empty())
		{
			LOG(INFO) << "The Disk have no file";
			break;
		}

		DiskInfo temp_info;
	    GetOneDiskInfo(disk_path_name, temp_info);

		//如果检测到硬盘使用率小于检测值，就退出循环
		if (temp_info.use_rate < kDiskExcateRate) break;
		
		uint64 delete_size = GalcDeleteSize(temp_info);

		LOG(INFO) << "Should Delete Size " << delete_size;

		TorrentAcessInfoSet& info_set  = iter->second;
		CleanSomeTorrentFile(info_set, delete_size);
	}
	
	LOG(INFO) << "Disk adjust thread finish";
}		

/*-----------------------------------------------------------------------------
 * 描  述: 调整Disk资源接口
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描� 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::AdjustResouce(const std::vector<DiskInfo>& adjust_disk_vec)
{
	LOG(INFO) << "Adjust  Disk  Resource ";

	for (const DiskInfo& info : adjust_disk_vec)
	{
		StartAdjustThread(info);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 监测Disk资源
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void DiskMonitor::Monitor()
{
	//LOG(INFO) << "Disk Monitor ";
	//Disk取瞬时值， 不涉及策略, 根据使用率 和调度标志进行调度
	std::vector<DiskInfo> disk_vec;
	std::vector<DiskInfo> adjust_disk_vec;
	GetInfo(disk_vec);

	AdjustFlagMap::iterator it;
	for (auto info : disk_vec)
	{		
		std::string disk_name(info.disk_name);
		it = adjust_flag_map_.find(disk_name);
		if (it == adjust_flag_map_.end()) continue;

		if (info.use_rate < kDiskExcateRate)
		{
			//小于kDiskExcateRate，调度标志无条件置为false
			it->second = false;
			continue;
		}

		if (it->second) continue;

		if (info.use_rate > kDiskOverloadRate)
		{
			it->second = true;
			adjust_disk_vec.push_back(info);
			continue;
		}
	}
	
	if (!adjust_disk_vec.empty())
	{
		AdjustResouce(adjust_disk_vec);
	}
}

//=============================NetMonitor================================

/*-----------------------------------------------------------------------------
 * 描  述: NetMonitor构造函数
 * 参  数: [in]ses SessionManager引用
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
NetMonitor::NetMonitor(SessionManager& ses): session_manager_(ses)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化函数 
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void NetMonitor::Init()
{
	GetNetStats(first_stats_vec_);
	GetNetStats(priv_stats_vec_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 查询Net资源使用数据
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void NetMonitor::GetInfo(std::vector<NetInfo>& info_vec)
{
	std::vector<NetStats> net_vec;
	GetNetStats(net_vec);

	//做DiskInfo的统计信息处理
	NetInfo temp_info;
	int cursor = 0;
	for (auto stats:net_vec)
	{
		strcpy(temp_info.net_name,stats.net_name);
		temp_info.tx_total_size = stats.out_bytes - first_stats_vec_[cursor].out_bytes;								
		temp_info.rx_total_size = stats.in_bytes - first_stats_vec_[cursor].in_bytes;
		temp_info.tx_moment_size = stats.out_bytes - priv_stats_vec_[cursor].out_bytes;
		temp_info.rx_moment_size = stats.in_bytes - priv_stats_vec_[cursor].in_bytes;

		info_vec.push_back(temp_info);
		cursor++;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 网络监控函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月07日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void NetMonitor::Monitor()
{
	//只做Net数据的实时更新 
	std::vector<NetStats> net_vec;
	GetNetStats(net_vec);
	for (uint32 i=0; i < net_vec.size(); i++)
	{
		UpdateNetStats(priv_stats_vec_[i], net_vec[i]);
	}
}

//=============================MonitorManager================================

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
MonitorManager::MonitorManager(SessionManager& session_man):
	session_manager_(session_man),
	stop_flag_(false),
	cpu_monitor_(session_man),
	mem_monitor_(session_man),
	disk_monitor_(session_man),
	net_monitor_(session_man),
	monitor_ios_(),
	timer_(monitor_ios_, boost::bind(&MonitorManager::Monitor, this), kTimerClock)
{	
	Init(); //初始化操作
}

/*-----------------------------------------------------------------------------
 * 描  述: 监控函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::Monitor()
{
	cpu_monitor_.Monitor();
	mem_monitor_.Monitor();
	disk_monitor_.Monitor();
	net_monitor_.Monitor();
}

/*-----------------------------------------------------------------------------
 * 描  述: 启动io_service
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::IoServiceThread(io_service& ios, std::string thread_name)
{
	LOG(INFO) << "Start " << thread_name << " thread...";

	do
	{
		ios.run();
		ios.reset();
	}while (!stop_flag_);

	LOG(INFO) << "Stop " << thread_name << " thread...";
}

/*-----------------------------------------------------------------------------
 * 描  述:启动接口
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::Run()
{
	// 启动定时器线程
	monitor_thread_ = boost::thread(boost::bind(&MonitorManager::IoServiceThread, 
		this, boost::ref(monitor_ios_), "monitor tick"));

	// 启动定时器
	timer_.Start();
}

/*-----------------------------------------------------------------------------
 * 描  述: 初始化接口
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::Init()
{
	cpu_monitor_.Init();
	net_monitor_.Init();
	disk_monitor_.Init();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取Cpu信息
 * 参  数: [out] info_vec Cpu信息传出向量
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::GetCpuInfo(std::vector<CpuInfo>& info_vec)
{
	cpu_monitor_.GetInfo(info_vec);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取Cpu信息
 * 参  数: [out] info 传出Mem信息
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::GetMemInfo(MemInfo& info)
{
	mem_monitor_.GetInfo(info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取Disk信息
 * 参  数: [out] info_vec Disk信息传出向量
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::GetDiskInfo(std::vector<DiskInfo>& info_vec)
{
	disk_monitor_.GetInfo(info_vec);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取Net信息
 * 参  数: [out] info_vec Net信息传出向量
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void MonitorManager::GetNetInfo(std::vector<NetInfo>& info_vec)
{
	net_monitor_.GetInfo(info_vec);
}

}


