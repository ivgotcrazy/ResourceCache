/*#############################################################################
 * �ļ���   : get_bcrm_info.hpp
 * ������   : vicent_pan	
 * ����ʱ�� : 2013/11/1
 * �ļ����� : bcrm info ��Ϣ��ȡ������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��:torrent �µ���Դ��Ϣ
 * ��  ��: vincent_pan
 * ʱ  ��: 2013��11��1��
 ----------------------------------------------------------------------------*/ 
struct TorrentResourceInfo		 
{
	InfoHashSP info_hash;		//��torrent_id ��Ӧ
	uint32 complete_precent;	//��ɰٷֱ�
	uint32 inner_peer_num;		//�ڲ�peer������
	uint32 outer_peer_num;		//�ⲿpeer������
	uint32 alive_time;			//���ʱ��
	uint64 upload_bandwidth;	//�ϴ�����
	uint64 download_bandwidth;	//���ش���
	uint64 total_upload;		//���ϴ���С
	uint64 total_download;		//�����ش�С
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
 * ��  ��: session �µ���Դ��Ϣ
 * ��  ��: vincent_pan
 * ʱ  ��: 2013��11��1��
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
 * ��  ��: peer �µ���Դ��Ϣ
 * ��  ��: vincent_pan
 * ʱ  ��: 2013��11��1��
 ----------------------------------------------------------------------------*/ 
struct PeerResourceInfo
{
	unsigned long peer_ip ;                //peer ��ip��ַ
	uint32 client_type;                    //�ͻ�������
	std::string peer_type;                 //peer ��peer����
    uint32 complete_precent;               //��ɰٷֱ�
    uint32 alive_time;                     //���ʱ��
    uint64 upload_bandwidth;               //�ϴ�����
	uint64 download_bandwidth;             //���ش���
    uint64 upload;                         //�ϴ���С
    uint64 download;                       //���ش�С
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
 * ����: bcrm ����� �����ϵͳ��ȡ��Ӧ������
 * ���ߣ�vincent_pan
 * ʱ�䣺2013/11/12
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
	SessionManager & session_manager_;                               //sessionmanger ����
	MonitorManager & monitor_manager_;  

	TorrentResourceInfo torrent_resrc_info_;   
 	SessionResourceInfo session_resrc_info_;
	PeerResourceInfo peer_resrc_info_;                              // peer�µ���Դ��Ϣ

	friend class BcrmServer; 
};

}
#endif
