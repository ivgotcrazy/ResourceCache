/*#############################################################################
 * �ļ���   : get_bcrm_info.cpp
 * ������   : vicent_pan	
 * ����ʱ�� : 2013/11/1
 * �ļ����� : bcrm info ��Ϣ��ȡ��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#include "get_bcrm_info.hpp"
#include "protobuf_msg_encode.hpp"
#include "bc_util.hpp"
#include "bt_peer_connection.hpp"

namespace BroadCache

{

/*-----------------------------------------------------------------------------
 * ��  ��: GetBcrmInfo�Ĺ��캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
GetBcrmInfo::GetBcrmInfo(SessionManager & sm, MonitorManager & mm):session_manager_(sm),														    monitor_manager_(mm)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����session����Դ��Ϣ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: ��ȡ��Ӧsession�µ�torrent��Դ��Ϣ
 * ��  ��: [in] session_type ��ʾ��Ӧsession
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: ��ȡϵͳ��Դ��Ϣ
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: ��ȡ��Ӧtorrent�µ�peer��Ϣ
 * ��  ��: [in] session type  session���Ͳ��Ҷ�Ӧ��session
 		   [in] info_hash     info_hashֵ���Ҷ�Ӧ��torrent
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
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
	
	MergerInfo(peer_info);  //�ϲ�
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡIoCache��Դ��Ϣ
 * ��  ��: [in] session_type
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: �ϲ�info��Ϣ�γɱ���
 * ��  ��: [in] info  ��Ҫ�ϲ�info��Ϣ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::MergerInfo(PbMessage & info)
{
	info_length_ = GetProtobufMessageEncodeLength(info);
	info_.clear();
	info_.resize(info_length_);
	EncodeProtobufMessage(info, &(info_[0]), info_length_);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����session����Դ��Ϣ
 * ��  ��: [out] siv
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetAllSessionInfo(SessionResourceInfoVec & siv)
{
	for(uint32 it = 0; it < session_manager_.session_vec_.size(); ++it)
	{
		siv.push_back(GetSessionResourceInfo(session_manager_.session_vec_[it]));
	}	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����session����Դ��Ϣ
 * ��  ��: [in] session_type
 		   [out] tiv
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: ��ȡcache ״̬��Ϣ
 * ��  ��: [in] session_type
 		   [out] cs
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: ��ȡcpu����Դ��Ϣ
 * ��  ��: [out] civ
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetCpuInfo(std::vector<CpuInfo> & civ)
{
	monitor_manager_.GetCpuInfo(civ);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡmemery����Դ��Ϣ
 * ��  ��: [out] mi
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetMemInfo(MemInfo & mi)
{
	monitor_manager_.GetMemInfo(mi);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡnet��Դ��Ϣ
 * ��  ��: [out] niv
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetNetInfo(std::vector<NetInfo> & niv)
{
	monitor_manager_.GetNetInfo(niv);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡdisk����Դ��Ϣ
 * ��  ��: [out] div
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void GetBcrmInfo::GetDiskInfo(std::vector<DiskInfo> & div)
{	
	monitor_manager_.GetDiskInfo(div);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡtorrent�µ�peer��Դ��Ϣ
 * ��  ��: [in] session_type
 		   [in] info_hash
 		   [out] piv
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: ��ȡsession����Դ��Ϣ
 * ��  ��: [in] se
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
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
 * ��  ��: ��ȡtorrent����Դ��Ϣ
 * ��  ��: [in] t 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
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
	torrent_resrc_info_.inner_peer_num = 0;    //��ʼ��
	torrent_resrc_info_.outer_peer_num = 0;    //��ʼ��
    t->policy_->GetInOutPeerNum(&torrent_resrc_info_.inner_peer_num, &torrent_resrc_info_.outer_peer_num);

	return torrent_resrc_info_;

}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡpeer����Դ��Ϣ
 * ��  ��: [in] peer
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� vincent_pan
 *   ���� ����
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
