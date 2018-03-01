/*#############################################################################
 * ÎÄ¼şÃû   : monitor_manager.hpp
 * ´´½¨ÈË   : tom_liu	
 * ´´½¨Ê±¼ä : 2013Äê10ÔÂ24ÈÕ
 * ÎÄ¼şÃèÊö : SystemResourceMonitorÀàÊµÏÖ
 * °æÈ¨ÉùÃ÷ : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "monitor_manager.hpp"
#include "basic_monitor.hpp"
#include "session_manager.hpp"

namespace BroadCache
{
const uint32 kCpuOverloadRate = 90;
const uint32 kMemOverloadRate = 95;
const uint32 kDiskOverloadRate = 90; //Ó²ÅÌ¹ıÔØÊ¹ÓÃÂÊ
const uint32 kDiskExcateRate = 80; //Ó²ÅÌ±¥ÂúÊ¹ÓÃÂÊ
const int64 kAdjustExpireTime = 20; //×ÊÔ´µ÷¶È×î¶Ì¼ä¸ôÊ±¼ä
const uint32 kAverageNum = 5; //¼ÆËãÆ½¾ùÊ¹ÓÃÂÊ£¬±£ÁôÔÚqueueÖĞ¼ÇÂ¼µÄ¸öÊı
const uint32 kTimerClock = 10; //¶¨Ê±Æ÷¼ä¸ôÊ±¼ä

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ¼ÆËãÆ½¾ùÖµº¯Êı
 * ²Î  Êı: rate_queue  ±£´æÊıÖµµÄ¶ÓÁĞ
 * ·µ»ØÖµ: rate ×îĞÂÒ»´ÎµÄÊ¹ÓÃÂÊ
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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

	//±£Áô5´Î¼ÇÂ¼£¬È¥µôÒ»¸ö×î¸ßÖµ£¬È¥µôÒ»¸ö×îµÍÖµ,È¡Æ½¾ùÖµ
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
 * Ãè  Êö: CpuMonitor¹¹Ôìº¯Êı
 * ²Î  Êı: [in]ses SessionManagerÒıÓÃ
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
CpuMonitor::CpuMonitor(SessionManager& session_man): session_manager_(session_man)
													   , cpu_adjust_flag_(false)
{
}

void CpuMonitor::Init()
{
	LOG(INFO) << "Cpu Monitor Init";
	//»ñÈ¡³õÊ¼cpuÊı¾İ
	GetCpuStats(priv_stats_vec_);
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ²éÑ¯CpuÊ¹ÓÃÇé¿ö
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void CpuMonitor::GetInfo(std::vector<CpuInfo>& info_vec)
{
	std::vector<CpuStats> now_stats_vec;
	GetCpuStats(now_stats_vec);

	CpuInfo info;
	for (uint32 i=0; i < now_stats_vec.size(); i++)
	{
		strcpy(info.cpu_name, priv_stats_vec_[i].cpu_name);

		//jiffiesÊÇÄÚºËÖĞµÄÒ»¸öÈ«¾Ö±äÁ¿£¬ÓÃÀ´¼ÇÂ¼×ÔÏµÍ³Æô¶¯Ò»À´²úÉúµÄ½ÚÅÄÊı
		uint64 priv_total_jiffies = CountJiffies(priv_stats_vec_[i]);
		uint64 now_total_jiffies = CountJiffies(now_stats_vec[i]);
		//LOG(INFO) << "priv:" << priv_total_jiffies << "now" << now_total_jiffies;
		uint64 total_jiffies = now_total_jiffies - priv_total_jiffies;
		uint64 idle_total_jiffies = now_stats_vec[i].cpu_idle - priv_stats_vec_[i].cpu_idle;
		uint32 use_rates = 0;
		if (total_jiffies != 0) //¿ÉÄÜ·¢Éú³ı0´íÎó
		{
			use_rates = 100 - ((idle_total_jiffies * 100) / total_jiffies);
		}
		if (use_rates > 100 ) use_rates = 100;
		
		info.use_rate = use_rates;
		info_vec.push_back(info);

		//²éÑ¯Íêºó¸üĞÂ priv_stats_vecµÄÖµ
		UpdateCpuStats(priv_stats_vec_[i],now_stats_vec[i]);
	}	
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: µ÷ÕûCpu×ÊÔ´½Ó¿Ú
 * ²Î  Êı: block ture±í×èÈû false±í»Ö¸´
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
 * Ãè  Êö: ¼à²âCpu×ÊÔ´
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void CpuMonitor::Monitor()
{ 
	LOG(INFO) << "Cpu Monitor -----------------";
	//std::cout << "Init Time: " << cpu_adjust_time_  << " Now Time: " << TimeNow();
	
	//Èç¹ûµ÷¿ØÊ±¼ä¼ä¾à²»³¬¹ı×î¶ÌÊ±¼ä£¬Ö±½ÓÌø¹ı
	int64 seconds = GetDurationSeconds(cpu_adjust_time_, TimeNow());
	if (seconds < kAdjustExpireTime) return;

	std::vector<CpuInfo> info_vec;
    GetInfo(info_vec);
	if (info_vec.empty()) return;
	uint32 real_rate = CalcAverage(rate_queue_, info_vec[0].use_rate); //Ö»ĞèÈ¡ÕûÌåcpuĞÅÏ¢
	if (real_rate >= kCpuOverloadRate)
	{
		cpu_adjust_time_ = TimeNow();
		cpu_adjust_flag_ = true;
		//AdjustResouce(true);
	}
	else  //Ê¹ÓÃÂÊÕı³£ºó£¬ ÖØÖÃsessionµÄÏà¹Ø±êÊ¶
	{
		cpu_adjust_flag_ = false;
		//AdjustResouce(false);
	}
	
}

//=============================MemMonitor================================

/*-----------------------------------------------------------------------------
 * Ãè  Êö: MemMonitor¹¹Ôìº¯Êı
 * ²Î  Êı: [in]ses SessionManagerÒıÓÃ
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
MemMonitor::MemMonitor(SessionManager& ses): session_manager_(ses) ,mem_adjust_flag_(false)
{
}


/*-----------------------------------------------------------------------------
 * Ãè  Êö: ²éÑ¯MemÊ¹ÓÃÇé¿ö
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
 * Ãè  Êö: µ÷ÕûMem×ÊÔ´½Ó¿Ú
 * ²Î  Êı: block ture±í×èÈû false±í»Ö¸´
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
 * Ãè  Êö: ¼à²âMem×ÊÔ´
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MemMonitor::Monitor()
{ 
	MemInfo meminfo;
	uint32 real_rate;
	GetInfo(meminfo);
	
	real_rate = CalcAverage(rate_queue_, meminfo.use_rate);

	//Èç¹ûµ÷¿ØÊ±¼ä¼ä¾à²»³¬¹ı×î¶ÌÊ±¼ä£¬Ö±½ÓÌø¹ı
	int64 seconds = GetDurationSeconds(mem_adjust_time_, TimeNow());
	if (seconds < kAdjustExpireTime) return;
	
	if (real_rate > kMemOverloadRate)
	{
		mem_adjust_time_ = TimeNow();
		mem_adjust_flag_ = true;
		AdjustResouce(true);
	}
	else if (mem_adjust_flag_) //Ê¹ÓÃÂÊÕı³£ºó£¬ ÖØÖÃsessionµÄÏà¹Ø±êÊ¶
	{
		mem_adjust_flag_ = false;
		AdjustResouce(false);
	}
}

//=============================DiskMonitor================================

/*-----------------------------------------------------------------------------
 * Ãè  Êö: DiskMonitor¹¹Ôìº¯Êı
 * ²Î  Êı: ses SessionManagerÒıÓÃ
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
DiskMonitor::DiskMonitor(SessionManager& ses): session_manager_(ses)
{
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ³õÊ¼»¯º¯Êı
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void DiskMonitor::Init()
{
	//³õÊ¼»¯ adjust_flag_map_ 
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
 * Ãè  Êö: ²éÑ¯DiskÊ¹ÓÃÇé¿ö
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void DiskMonitor::GetInfo(std::vector<DiskInfo>& info_vec)
{
    std::vector<DiskStats> disk_vec;
	GetDiskStats(disk_vec);

	//×öDiskInfoµÄÍ³¼ÆĞÅÏ¢´¦Àí
	DiskInfo temp_info;
	for (auto stats : disk_vec)
	{
		strcpy(temp_info.disk_name, stats.disk_name);
		temp_info.total_size = (stats.total_blocks * stats.block_size) / (1024 * 1024 * 1024); 
		
		//Ê¹ÓÃÂÊ¼ÆËã, ÓÃ free_blocks Óë df ÃüÁîÒ»ÖÂ
		temp_info.use_rate = 100 - (stats.free_blocks * 100 / stats.total_blocks);
		
		//LOG(INFO) << "use  size:" << (stats.total_blocks-stats.free_blocks)*4/1024; 
		info_vec.push_back(temp_info);
	}
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ¸ù¾İ´ÅÅÌÃû»ñÈ¡Ò»¸ö´ÅÅÌµÄĞÅÏ¢
 * ²Î  Êı: [in] disk_name ´ÅÅÌÃû 
 		 [out] info  DiskĞÅÏ¢
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
 * Ãè  Êö: ¸üĞÂÎÄ¼ş·ÃÎÊ¼¯ºÏ
 * ²Î  Êı: [in] disk_path_name Ó²ÅÌÃû
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ12ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void DiskMonitor::UpdateAcessSet(const std::string disk_path_name)
{
	//¼ÆËã×îĞÂµÄaccess_info_map_
	TorrentAcessInfoMap::iterator it;
	it = access_info_map_.find(disk_path_name);
	if (it != access_info_map_.end() &&  !it->second.empty())
	{
		return ; //TorrentAcessInfoSetÖĞÓĞÖµ¾ÍÖ±½ÓÍË³ö
	}

	std::vector<std::string> out_path_vec;
	session_manager_.CalcShouldDeletePath(disk_path_name, out_path_vec);

	error_code ec;
	if (it->second.empty())  // ¸üĞÂ setÖĞµÄÊı¾İ
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
			   		if (!fs::is_directory(item->path())) continue; //²»ÊÇÄ¿Â¼¾Í²»ÊÇ±£´ætorrentµÄÄ¿Â¼
					LOG(INFO) << "Update acess info | path:" << item->path().string();
					if (session_manager_.IsPathInUse(item->path().leaf().string()))  continue;
					AddAcessInfo(item->path().leaf().string(), path, it->second);
			   }
			}
		}		
	}
	else  //ĞÂÔöset
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
			   		if (!fs::is_directory(item->path())) continue; //²»ÊÇÄ¿Â¼¾Í²»ÊÇ±£´ætorrentµÄÄ¿Â¼
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
 * Ãè  Êö: ĞÂÔöÒ»¸öÎÄ¼ş·ÃÎÊĞÅÏ¢
 * ²Î  Êı: [in] directory torrent±£´æÄ¿Â¼ 
 		 [in] path  ¸¸Â·¾¶
 		 [out] info_set ±£´æÎÄ¼şĞÅÏ¢µÄ¼¯ºÏ
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ12ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
 * Ãè  Êö: ÇåÀíÓ²ÅÌÉÏµÄ²¿·ÖTorrentÏà¹ØÎÄ¼ş
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ12ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
		//¼ÇÂ¼¶Á²»ÄÜ±»É¾µÄÇé¿ö
		if (ec && ec.value() != boost::system::errc::device_or_resource_busy)
		{
			LOG(INFO) << "Can't delete directory | reason:" << ec.message();
		}
		
		cur_iterator++;
	}	
	
	info_set.erase(info_set.begin(), cur_iterator);
}	

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ¿ªÆôµ÷¿ØÏß³ÌÓÃÓÚµ÷¶ÈÃ¿Ò»¿éÓ²ÅÌ
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ12ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void DiskMonitor::StartAdjustThread(const DiskInfo& info)
{
	boost::thread t(boost::bind(&DiskMonitor::AdjustOneDisk, this, info));
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡µ±Ç°Ó²ÅÌµÄµ÷¶È±êÖ¾
 * ²Î  Êı: [in] disk_path_name  Ó²ÅÌÃû
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ12ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
bool DiskMonitor::GetDiskAdjustFlag(std::string disk_path_name)
{
	AdjustFlagMap::iterator it;
	it = adjust_flag_map_.find(disk_path_name);
	if (it == adjust_flag_map_.end()) return false;

	return it->second;
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ¼ÆËãÒªÉ¾³ıµÄ´ÅÅÌ¿Õ¼ä´óĞ¡
 * ²Î  Êı: [in] info  Ó²ÅÌĞÅÏ¢
 * ·µ»ØÖµ: uint64  ¿Õ¼ä´óĞ¡
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ12ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
inline uint64 DiskMonitor::GalcDeleteSize(const DiskInfo& info)
{
	uint64 g_unit = (((info.use_rate - kDiskExcateRate) * 100) / 100 * info.total_size + 1) / 100;
	uint64 kb_unit = g_unit * 1024 * 1024;
	return kb_unit;
}



/*-----------------------------------------------------------------------------
 * Ãè  Êö: µ÷ÕûÒ»¸öDisk×ÊÔ´½Ó¿Ú
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ12ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void DiskMonitor::AdjustOneDisk(const DiskInfo& info)
{
	LOG(INFO) << "Start new thread adjust one disk resource";
	std::string disk_path_name(info.disk_name);

	while (GetDiskAdjustFlag(disk_path_name))
	{	
		LOG(INFO) << "Adjust disk loop begin";
		//¸üĞÂaccess_info_map_
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

		//Èç¹û¼ì²âµ½Ó²ÅÌÊ¹ÓÃÂÊĞ¡ÓÚ¼ì²âÖµ£¬¾ÍÍË³öÑ­»·
		if (temp_info.use_rate < kDiskExcateRate) break;
		
		uint64 delete_size = GalcDeleteSize(temp_info);

		LOG(INFO) << "Should Delete Size " << delete_size;

		TorrentAcessInfoSet& info_set  = iter->second;
		CleanSomeTorrentFile(info_set, delete_size);
	}
	
	LOG(INFO) << "Disk adjust thread finish";
}		

/*-----------------------------------------------------------------------------
 * Ãè  Êö: µ÷ÕûDisk×ÊÔ´½Ó¿Ú
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   Ãèö ´´½¨
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
 * Ãè  Êö: ¼à²âDisk×ÊÔ´
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void DiskMonitor::Monitor()
{
	//LOG(INFO) << "Disk Monitor ";
	//DiskÈ¡Ë²Ê±Öµ£¬ ²»Éæ¼°²ßÂÔ, ¸ù¾İÊ¹ÓÃÂÊ ºÍµ÷¶È±êÖ¾½øĞĞµ÷¶È
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
			//Ğ¡ÓÚkDiskExcateRate£¬µ÷¶È±êÖ¾ÎŞÌõ¼şÖÃÎªfalse
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
 * Ãè  Êö: NetMonitor¹¹Ôìº¯Êı
 * ²Î  Êı: [in]ses SessionManagerÒıÓÃ
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
NetMonitor::NetMonitor(SessionManager& ses): session_manager_(ses)
{
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ³õÊ¼»¯º¯Êı 
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void NetMonitor::Init()
{
	GetNetStats(first_stats_vec_);
	GetNetStats(priv_stats_vec_);
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ²éÑ¯Net×ÊÔ´Ê¹ÓÃÊı¾İ
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void NetMonitor::GetInfo(std::vector<NetInfo>& info_vec)
{
	std::vector<NetStats> net_vec;
	GetNetStats(net_vec);

	//×öDiskInfoµÄÍ³¼ÆĞÅÏ¢´¦Àí
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
 * Ãè  Êö: ÍøÂç¼à¿Øº¯Êı
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê11ÔÂ07ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void NetMonitor::Monitor()
{
	//Ö»×öNetÊı¾İµÄÊµÊ±¸üĞÂ 
	std::vector<NetStats> net_vec;
	GetNetStats(net_vec);
	for (uint32 i=0; i < net_vec.size(); i++)
	{
		UpdateNetStats(priv_stats_vec_[i], net_vec[i]);
	}
}

//=============================MonitorManager================================

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ¹¹Ôìº¯Êı
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
	Init(); //³õÊ¼»¯²Ù×÷
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ¼à¿Øº¯Êı
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MonitorManager::Monitor()
{
	cpu_monitor_.Monitor();
	mem_monitor_.Monitor();
	disk_monitor_.Monitor();
	net_monitor_.Monitor();
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: Æô¶¯io_service
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
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
 * Ãè  Êö:Æô¶¯½Ó¿Ú
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MonitorManager::Run()
{
	// Æô¶¯¶¨Ê±Æ÷Ïß³Ì
	monitor_thread_ = boost::thread(boost::bind(&MonitorManager::IoServiceThread, 
		this, boost::ref(monitor_ios_), "monitor tick"));

	// Æô¶¯¶¨Ê±Æ÷
	timer_.Start();
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: ³õÊ¼»¯½Ó¿Ú
 * ²Î  Êı: 
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MonitorManager::Init()
{
	cpu_monitor_.Init();
	net_monitor_.Init();
	disk_monitor_.Init();
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡CpuĞÅÏ¢
 * ²Î  Êı: [out] info_vec CpuĞÅÏ¢´«³öÏòÁ¿
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MonitorManager::GetCpuInfo(std::vector<CpuInfo>& info_vec)
{
	cpu_monitor_.GetInfo(info_vec);
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡CpuĞÅÏ¢
 * ²Î  Êı: [out] info ´«³öMemĞÅÏ¢
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MonitorManager::GetMemInfo(MemInfo& info)
{
	mem_monitor_.GetInfo(info);
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡DiskĞÅÏ¢
 * ²Î  Êı: [out] info_vec DiskĞÅÏ¢´«³öÏòÁ¿
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MonitorManager::GetDiskInfo(std::vector<DiskInfo>& info_vec)
{
	disk_monitor_.GetInfo(info_vec);
}

/*-----------------------------------------------------------------------------
 * Ãè  Êö: »ñÈ¡NetĞÅÏ¢
 * ²Î  Êı: [out] info_vec NetĞÅÏ¢´«³öÏòÁ¿
 * ·µ»ØÖµ:
 * ĞŞ  ¸Ä:
 *   Ê±¼ä 2013Äê10ÔÂ24ÈÕ
 *   ×÷Õß tom_liu
 *   ÃèÊö ´´½¨
 ----------------------------------------------------------------------------*/
void MonitorManager::GetNetInfo(std::vector<NetInfo>& info_vec)
{
	net_monitor_.GetInfo(info_vec);
}

}


