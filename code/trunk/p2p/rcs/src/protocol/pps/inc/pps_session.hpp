/*#############################################################################
 * �ļ���   : pps_session.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��25��
 * �ļ����� : BtSession������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef PPS_SESSION_HEAD
#define PPS_SESSION_HEAD

#include "session.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: PPSЭ��Session
 * ���ߣ�tom_liu
 * ʱ�䣺2013��12��25��
 *****************************************************************************/
class PpsSession : public Session
{
public:
	PpsSession();
	~PpsSession();

	// ������������
	virtual PeerConnectionSP CreatePeerConnection(const SocketConnectionSP& conn) override;

	// ������������
	virtual PeerConnectionSP CreatePeerConnection(const EndPoint& remote, PeerType peer_type) override;

    // ��ȡ�ͻ��˵�peer-id�ֽ���
    static constexpr uint32 GetPeerIdLength();

    // ��ȡ���ؿͻ��˵�20λpeer-id
    static const char* GetLocalPeerId();

private:

	// ������������Socket�����Ĵ�����ͳһʹ��Session::OnReceiveData
	virtual bool CreateSocketServer(SocketServerVec& servers);

	// ���������torrent
	virtual TorrentSP CreateTorrent(const InfoHashSP& hash, const fs::path& save_path);

	// ����·����ȡtorrent
	virtual InfoHashVec GetTorrentInfoHash(const fs::path& save_path);

	// ��ȡsession�洢·���б�
	virtual SavePathVec GetSavePath() override;

	// ��ȡsession��Э������
	virtual std::string GetSessionProtocol() override;

	//����keep_alive��Ϣ
	virtual void SendKeepAlive() override;

    // �ϱ�������Դ
    void ReportResource();

    // ��UGS�����Ϻ�Ļص�����
    void OnUgsConnected();

	void ParseSavePath();

private:
	SavePathVec save_paths_;
};

}

#endif

