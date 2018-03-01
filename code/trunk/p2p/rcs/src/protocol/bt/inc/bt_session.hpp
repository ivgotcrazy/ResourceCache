/*#############################################################################
 * 文件名   : bt_session.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年09月13日
 * 文件描述 : BtSession类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "session.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: BT协议Session
 * 作者：teck_zhou
 * 时间：2013年08月31日
 *****************************************************************************/
class BtSession : public Session
{
public:
	BtSession();
	~BtSession();

	// 创建被动连接
	virtual PeerConnectionSP CreatePeerConnection(const SocketConnectionSP& conn) override;

	// 创建主动连接
	virtual PeerConnectionSP CreatePeerConnection(const EndPoint& remote, PeerType peer_type) override;

    // 获取客户端的peer-id字节数
    static constexpr uint32 GetPeerIdLength();

    // 获取本地客户端的20位peer-id
    static const char* GetLocalPeerId();

private:

	// 创建服务器端Socket，报文处理函数统一使用Session::OnReceiveData
	virtual bool CreateSocketServer(SocketServerVec& servers);

	// 创建具体的torrent
	virtual TorrentSP CreateTorrent(const InfoHashSP& hash, const fs::path& save_path);

	// 根据路径获取torrent
	virtual InfoHashVec GetTorrentInfoHash(const fs::path& save_path);

	// 获取session存储路径列表
	virtual SavePathVec GetSavePath() override;

	// 获取session的协议类型
	virtual std::string GetSessionProtocol() override;

	//发送keep_alive消息
	virtual void SendKeepAlive() override;

    // 上报本地资源
    void ReportResource();

    // 当UGS连接上后的回调函数
    void OnUgsConnected();

private:
	void ParseSavePath();

private:
	SavePathVec save_paths_;
};

}
