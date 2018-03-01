/*#############################################################################
 * 文件名   : get_bcrm_info.hpp
 * 创建人   : vicent_pan	
 * 创建时间 : 2013/11/1
 * 文件描述 : bcrm info 信息获取类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef GET_BCRM_INFO_HEAD
#define GET_BCRM_INFO_HEAD

#include "session_manager.hpp"
#include "depend.hpp"
#include "bcrm_message.pb.h"
#include "monitor_manager.hpp"
#include "policy.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述:torrent 下的资源信息
 * 作  者: vincent_pan
 * 时  间: 2013年11月1日
 ----------------------------------------------------------------------------*/ 
struct TorrentResourceInfo		 
{
	InfoHashSP info_hash;		//和torrent_id 对应
	uint32 complete_precent;	//完成百分比
	uint32 inner_peer_num;		//内部peer的数量
	uint32 outer_peer_num;		//外部peer的数量
	uint32 alive_time;			//存活时间
	uint64 upload_bandwidth;	//上传带宽
	uint64 download_bandwidth;	//下载带宽
	uint64 total_upload;		//总上传大小
	uint64 total_download;		//总下载大小
	void operator=(const TorrentResourceInfo& rhs)
	{
		info_hash = rhs.info_hash;
		complete_precent = rhs.complete_precent;
		inner_peer_num = rhs.inner_peer_num;
		outer_peer_num = rhs.outer_peer_num;
		alive_time = rhs.alive_time;
		upload_bandwidth = rhs.upload_bandwidth;
		download_bandwidth = rhs.download_bandwidth;
		total_upload = rhs.total_upload;
		total_download = rhs.total_download;
	}
};
	
typedef std::vector<TorrentResourceInfo> TorrentResourceInfoVec;

/*-----------------------------------------------------------------------------
 * 描  述: session 下的资源信息
 * 作  者: vincent_pan
 * 时  间: 2013年11月1日
 ----------------------------------------------------------------------------*/ 
struct SessionResourceInfo {
	std::string session_type;
	uint32 torrent_num;
	uint32 peer_num;
	uint64 upload_bandwidth;
	uint64 download_bandwidth;
};    

typedef std::vector<SessionResourceInfo> SessionResourceInfoVec;

/*-----------------------------------------------------------------------------
 * 描  述: peer 下的资源信息
 * 作  者: vincent_pan
 * 时  间: 2013年11月1日
 ----------------------------------------------------------------------------*/ 
struct PeerResourceInfo
{
	unsigned long peer_ip ;                //peer 的ip地址
	uint32 client_type;                    //客户端类型
	std::string peer_type;                 //peer 端peer类型
    uint32 complete_precent;               //完成百分比
    uint32 alive_time;                     //存活时间
    uint64 upload_bandwidth;               //上传带宽
	uint64 download_bandwidth;             //下载带宽
    uint64 upload;                         //上传大小
    uint64 download;                       //下载大小
    void operator= (const PeerResourceInfo & rhs)
	{
		peer_ip =rhs.peer_ip;
		peer_type = rhs.peer_type;
		alive_time = rhs.alive_time;
		complete_precent = rhs.complete_precent;
		upload = rhs.upload;
		download = rhs.download;
	}
};

typedef std::vector<PeerResourceInfo> PeerResourceInfoVec;

/******************************************************************************
 * 描述: bcrm 服务端 负责从系统中取相应的数据
 * 作者：vincent_pan
 * 时间：2013/11/12
 *****************************************************************************/
class GetBcrmInfo:public boost::noncopyable
{

public:

	GetBcrmInfo(SessionManager & sm, MonitorManager & srm);

	void SetSessionInfo();
	void SetTorrentInfo(const std::string & session_type);
    void SetSystemInfo();
	void SetPeerInfo(const std::string & session_type, const std::string &info_hash);
	void SetCacheStatusInfo(const std::string & session_type);	

private:
	void MergerInfo(PbMessage & info);
	void GetAllSessionInfo(SessionResourceInfoVec & siv);
	void GetTorrentInfoOfSession(const std::string & session_type, TorrentResourceInfoVec & tiv);
	void GetCacheStatusInfo(const std::string & session_type, CacheStatus &cs);
	void GetCpuInfo(std::vector<CpuInfo> & civ);
	void GetMemInfo(MemInfo & mi);
	void GetNetInfo(std::vector<NetInfo> & niv);
	void GetDiskInfo(std::vector<DiskInfo> &div);

	void GetPeerInfoOfTorrent(const std::string & session_type, 
	                              const std::string &info_hash, 
	                                 PeerResourceInfoVec & piv);
	SessionResourceInfo GetSessionResourceInfo(const SessionSP & se);
	TorrentResourceInfo GetTorrentResourceInfo(const TorrentSP & t);
	PeerResourceInfo GetPeerResourceInfo(const PeerConnectionSP & peer);	 

private:

	std::vector<char> info_;
    uint32 info_length_;
	SessionManager & session_manager_;                               //sessionmanger 对象
	MonitorManager & monitor_manager_;  

	TorrentResourceInfo torrent_resrc_info_;   
 	SessionResourceInfo session_resrc_info_;
	PeerResourceInfo peer_resrc_info_;                              // peer下的资源信息

	friend class BcrmServer; 
};

}
#endif
