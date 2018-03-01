/*#############################################################################
 * �ļ���   : bt_session.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��13��
 * �ļ����� : BtSession������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "session.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����: BTЭ��Session
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��08��31��
 *****************************************************************************/
class BtSession : public Session
{
public:
	BtSession();
	~BtSession();

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

private:
	void ParseSavePath();

private:
	SavePathVec save_paths_;
};

}
