/*#############################################################################
 * 文件名   : get_bcrm_info.cpp
 * 创建人   : vicent_pan	
 * 创建时间 : 2013/11/1
 * 文件描述 : bcrm info 信息获取类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#include "get_bcrm_info.hpp"
#include "protobuf_msg_encode.hpp"
#include "bc_util.hpp"
#include "bt_peer_connection.hpp"

namespace BroadCache

{

/*-----------------------------------------------------------------------------
 * 描  述: GetBcrmInfo的构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
GetBcrmInfo::GetBcrmInfo(SessionManager & sm, MonitorManager & mm):session_manager_(sm),														    monitor_manager_(mm)
{
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取所有session的资源信息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::SetSessionInfo()
{
	BcrmSessionInfo session_info;
	SessionResourceInfoVec siv;
	GetAllSessionInfo(siv);
	for(uint32 it = 0; it < siv.size(); ++it)
	{	
		BcrmSessionInfo::SessionInfo *si = session_info.add_session_info();
		si->set_session_type(siv[it].session_type);
		si->set_torrent_num(siv[it].torrent_num);
		si->set_peer_num(siv[it].peer_num);
		si->set_upload_bandwidth(siv[it].upload_bandwidth);
		si->set_download_bandwidth(siv[it].download_bandwidth);
	}
	MergerInfo(session_info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取对应session下的torrent资源信息
 * 参  数: [in] session_type 表示对应session
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::SetTorrentInfo(const std::string & session_type)
{
	BcrmTorrentInfo torrent_info;

	TorrentResourceInfoVec tiv; 
	
    GetTorrentInfoOfSession(session_type, tiv);  
	
	for(uint32 it = 0; it < tiv.size(); ++it)
	{
		BcrmTorrentInfo::TorrentInfo *ti = torrent_info.add_torrent_info();
		ti->set_info_hash(tiv[it].info_hash->to_string());
		ti->set_complete_precent(tiv[it].complete_precent);
		ti->set_inner_peer_num(tiv[it].inner_peer_num);
		ti->set_outer_peer_num(tiv[it].outer_peer_num);
		ti->set_alive_time(tiv[it].alive_time);
		ti->set_upload_bandwidth(tiv[it].upload_bandwidth);
		ti->set_download_bandwidth(tiv[it].download_bandwidth);
		ti->set_total_upload(tiv[it].total_upload);
		ti->set_total_download(tiv[it].total_download);
	}

	MergerInfo(torrent_info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取系统资源信息
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::SetSystemInfo()
{
	BcrmSystemInfo system_info;

	BcrmSystemInfo::SystemInfo * sys = system_info.mutable_system_info();
	std::vector<CpuInfo> civ;
	GetCpuInfo(civ);

	for(uint32 i = 0; i < civ.size(); ++i)
	{	
		BcrmSystemInfo::SystemInfo::CpuInfo *ci = sys->add_cpu_info();
		ci->set_cpu_usage(civ[i].use_rate);
		ci->set_cpu_name(civ[i].cpu_name);
	}

	MemInfo mi;
	GetMemInfo(mi);
	sys->set_mem_total_size(mi.total_size);
	sys->set_mem_usage(mi.use_rate);
	
	std::vector<NetInfo> niv;
	GetNetInfo(niv);
	for(uint32 i = 0; i < niv.size(); ++i)
	{	
		BcrmSystemInfo::SystemInfo::NetInfo *ni = sys->add_net_info();
		ni->set_net_name(niv[i].net_name);
		ni->set_in_size(niv[i].rx_moment_size);
		ni->set_out_size(niv[i].tx_moment_size);
	}

	std::vector<DiskInfo>  div;
	GetDiskInfo(div);
	for(uint32 i = 0; i < div.size(); ++i)
	{
		BcrmSystemInfo::SystemInfo::DiskInfo *di = sys->add_disk_info();
		di->set_disk_name(div[i].disk_name);
		di->set_usage(div[i].use_rate);
		di->set_total_size(div[i].total_size);
	}

	MergerInfo(system_info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取对应torrent下的peer信息
 * 参  数: [in] session type  session类型查找对应的session
 		   [in] info_hash     info_hash值查找对应的torrent
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::SetPeerInfo(const std::string & session_type, const std::string &info_hash)
{
	BcrmPeerInfo peer_info;

	PeerResourceInfoVec piv;
	GetPeerInfoOfTorrent(session_type, info_hash, piv);
	for(uint32 it = 0; it < piv.size(); ++it)
	{
		BcrmPeerInfo::PeerInfo * pi = peer_info.add_peer_info();
		pi->set_peer_ip(piv[it].peer_ip);
		pi->set_client_type(piv[it].client_type);
		pi->set_peer_type(piv[it].peer_type);
		pi->set_complete_precent(piv[it].complete_precent);
		pi->set_alive_time(piv[it].alive_time);
		pi->set_upload_bandwidth(piv[it].upload_bandwidth);
		pi->set_download_bandwidth(piv[it].download_bandwidth);
		pi->set_upload(piv[it].upload);
		pi->set_download(piv[it].download);
	}
	
	MergerInfo(peer_info);  //合并
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取IoCache资源信息
 * 参  数: [in] session_type
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::SetCacheStatusInfo(const std::string & session_type)
{
	BcrmCacheStatusInfo iocache_info; 
	CacheStatus cs;
	GetCacheStatusInfo(session_type,cs);	
	BcrmCacheStatusInfo::CacheStatusInfo * ci = iocache_info.mutable_cache_status_info();
	ci->set_reads(cs.reads);
	ci->set_writes(cs.writes);
	ci->set_blocks_read(cs.blocks_read);
	ci->set_blocks_write(cs.blocks_write);
	ci->set_blocks_read_hit(cs.blocks_read_hit);
	ci->set_write_to_disk_directly(cs.write_to_disk_directly);
	ci->set_read_cache_size(cs.read_cache_size);
	ci->set_finished_write_cache_size(cs.finished_write_cache_size);
	ci->set_unfinished_write_cache_size(cs.unfinished_write_cache_size);
	MergerInfo(iocache_info);
}

/*-----------------------------------------------------------------------------
 * 描  述: 合并info信息形成报文
 * 参  数: [in] info  需要合并info信息
 * 返回值:
 * 修  改:
 *   时间 2013年10月24日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::MergerInfo(PbMessage & info)
{
	info_length_ = GetProtobufMessageEncodeLength(info);
	info_.clear();
	info_.resize(info_length_);
	EncodeProtobufMessage(info, &(info_[0]), info_length_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取所有session的资源信息
 * 参  数: [out] siv
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetAllSessionInfo(SessionResourceInfoVec & siv)
{
	for(uint32 it = 0; it < session_manager_.session_vec_.size(); ++it)
	{
		siv.push_back(GetSessionResourceInfo(session_manager_.session_vec_[it]));
	}	
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取所有session的资源信息
 * 参  数: [in] session_type
 		   [out] tiv
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetTorrentInfoOfSession(const std::string & session_type, TorrentResourceInfoVec & tiv)
{
	std::vector<SessionSP>::iterator iter = session_manager_.session_vec_.begin();
	for( ; iter != session_manager_.session_vec_.end(); ++iter)
	{
		if ((*iter)->session_type_ == session_type ) break;
	}
	if(iter == session_manager_.session_vec_.end()) return;
	
	auto it  = (*iter)->torrents_.begin();
	for( ; it != (*iter)->torrents_.end(); ++it)
	{
		tiv.push_back(GetTorrentResourceInfo(it->second));
		
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取cache 状态信息
 * 参  数: [in] session_type
 		   [out] cs
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetCacheStatusInfo(const std::string & session_type, CacheStatus &cs)
{
	for(uint32 it = 0; it < session_manager_.session_vec_.size(); ++it)
	{
		if (session_manager_.session_vec_[it]->session_type_ == session_type )
			cs = session_manager_.session_vec_[it]->io_thread_.GetCacheStatus();			
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取cpu的资源信息
 * 参  数: [out] civ
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetCpuInfo(std::vector<CpuInfo> & civ)
{
	monitor_manager_.GetCpuInfo(civ);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取memery的资源信息
 * 参  数: [out] mi
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetMemInfo(MemInfo & mi)
{
	monitor_manager_.GetMemInfo(mi);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取net资源信息
 * 参  数: [out] niv
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetNetInfo(std::vector<NetInfo> & niv)
{
	monitor_manager_.GetNetInfo(niv);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取disk的资源信息
 * 参  数: [out] div
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetDiskInfo(std::vector<DiskInfo> & div)
{	
	monitor_manager_.GetDiskInfo(div);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取torrent下的peer资源信息
 * 参  数: [in] session_type
 		   [in] info_hash
 		   [out] piv
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetPeerInfoOfTorrent(const std::string & session_type, 
	                               const std::string &info_hash, 
	                               PeerResourceInfoVec & piv)
{
	std::vector<SessionSP>::iterator iter = session_manager_.session_vec_.begin();
	for( ; iter != session_manager_.session_vec_.end(); ++iter)
	{
		if ((*iter)->session_type_ == session_type ) break;
	}
	if(iter == session_manager_.session_vec_.end()) return;
	
	auto it  = (*iter)->torrents_.begin();
	for( ; it != (*iter)->torrents_.end(); ++it)
	{
		if (it->second->info_hash()->to_string() == info_hash) break;
	}
	if(it == (*iter)->torrents_.end()) return;

	auto itp = it->second->policy_->peer_conn_map_.begin();
	for(; itp != it->second->policy_->peer_conn_map_.end(); ++itp)
	{
		piv.push_back(GetPeerResourceInfo(itp->second));
	}

}

/*-----------------------------------------------------------------------------
 * 描  述: 获取session的资源信息
 * 参  数: [in] se
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
SessionResourceInfo GetBcrmInfo::GetSessionResourceInfo(const SessionSP & se)
{
	session_resrc_info_.torrent_num = se->torrents_.size();
	session_resrc_info_.session_type = se->session_type_;
	auto it = se->torrents_.begin();
	session_resrc_info_.download_bandwidth = 0;
	session_resrc_info_.upload_bandwidth = 0;
	session_resrc_info_.peer_num = 0;
	for( ; it != se->torrents_.end(); ++it)
	{
		session_resrc_info_.peer_num += it->second->policy_->peer_conn_map_.size();

		session_resrc_info_.download_bandwidth += 
			it->second->policy_->GetDownloadBandwidth();

		session_resrc_info_.upload_bandwidth += 
			it->second->policy_->GetUploadBandwidth();
	}

	return session_resrc_info_;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取torrent的资源信息
 * 参  数: [in] t 
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
TorrentResourceInfo GetBcrmInfo::GetTorrentResourceInfo(const TorrentSP & t)
{
	torrent_resrc_info_.info_hash = t->torrent_info_.info_hash;

	if(t->torrent_info_.piece_map.size())
	{
		torrent_resrc_info_.complete_precent = 
			(t->torrent_info_.piece_map.count()* 100) /t->torrent_info_.piece_map.size();
	}
	torrent_resrc_info_.alive_time = t->alive_time_;       
	torrent_resrc_info_.total_upload = t->policy_->GetUploadTotalSize();
	torrent_resrc_info_.total_download = t->policy_->GetDownloadTotalSize();
	torrent_resrc_info_.upload_bandwidth = t->policy_->GetUploadBandwidth();
	torrent_resrc_info_.download_bandwidth = t->policy_->GetDownloadBandwidth();
	torrent_resrc_info_.inner_peer_num = 0;    //初始化
	torrent_resrc_info_.outer_peer_num = 0;    //初始化
    t->policy_->GetInOutPeerNum(&torrent_resrc_info_.inner_peer_num, &torrent_resrc_info_.outer_peer_num);

	return torrent_resrc_info_;

}

/*-----------------------------------------------------------------------------
 * 描  述: 获取peer的资源信息
 * 参  数: [in] peer
 * 返回值:
 * 修  改:
 *   时间 2013年11月12日
 *   作者 vincent_pan
 *   描述 创建
 ----------------------------------------------------------------------------*/
PeerResourceInfo GetBcrmInfo::GetPeerResourceInfo(const PeerConnectionSP & peer)
{
	peer_resrc_info_.peer_ip = peer->connection_id().remote.ip;
	auto btp = dynamic_cast<BtPeerConnection *>(peer.get());
	peer_resrc_info_.client_type = btp->peer_client_type();
	if(peer->peer_type() == PEER_INNER)
	{
		peer_resrc_info_.peer_type = "Inner";
	}
	else 
	{
		peer_resrc_info_.peer_type = "Outer";
	}

	if (peer->piece_map_.empty())
	{
		peer_resrc_info_.complete_precent = 0;
	}
	else
	{
		peer_resrc_info_.complete_precent = (peer->piece_map_.count() * 100) / peer->piece_map_.size();
	}
	peer_resrc_info_.alive_time = peer->alive_time_;               
	peer_resrc_info_.download = peer->GetDownloadSize();
	peer_resrc_info_.upload = peer->GetUploadSize();
	peer_resrc_info_.upload_bandwidth = peer->GetUploadBandwidth();  
	peer_resrc_info_.download_bandwidth = peer->GetDownloadBandwidth();
	return peer_resrc_info_;
}

}
