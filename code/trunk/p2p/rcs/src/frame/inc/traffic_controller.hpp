/*#############################################################################
 * 文件名   : traffic_controller.hpp
 * 创建人   : teck_zhou
 * 创建时间 : 2013年10月13日
 * 文件描述 : TrafficController类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef TRAFFIC_CONTROLLER_HEAD
#define TRAFFIC_CONTROLLER_HEAD

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "peer_request.hpp"
#include "piece_structs.hpp"
#include "connection.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

class PeerConnection;

/******************************************************************************
 * 描述: 连接带宽控制
 * 作者：teck_zhou
 * 时间：2013/10/12
 *****************************************************************************/
class TrafficController : public boost::noncopyable
{
public:
	TrafficController(PeerConnection* peer_conn, 
		              uint64 download_speed_limit, 
		              uint64 upload_speed_limit);
	void OnTick();

	void ReceivedDataRequest(const PeerRequest& request);
	void ReceivedDataResponse(const PeerRequest& request);

	void TrySendDataRequest();
	void TryProcPeerRequest();

	void StartDownload();
	void StopDownload();

	void StartUpload();
	void StopUpload();

	void OnSendDataResponse(ConnectionError error, const PeerRequest& request);

	uint64 GetDownloadSpeed() const;
	uint64 GetUploadSpeed() const;

	void set_download_limit(uint64 speed) { download_speed_limit_ = speed; }
	void set_upload_limit(uint64 speed) { upload_speed_limit_ = speed; }
	void set_fragment_size(uint64 size) { fragment_size_ = size; }

	uint64 GetTotalUploadSize(){ return total_upload_size_; }
	uint64 GetTotalDownloadSize(){ return total_download_size_; }

	PeerConnection* peer_conn() { return peer_conn_; }

private:
	struct PendingRequest
	{
		PeerRequest request;
		bool processing;
	};

	struct PendingFragment
	{
		PendingFragment() : frag_index(0), block_offset(0), frag_len(0), 
			tried_times(0), downloading(false), request_time(TimeNow())
		{
		}

		PieceBlock block;    // piece/block
		uint64 frag_index;   // fragment索引
		uint64 block_offset; // block首字节偏移
		uint64 frag_len;     // fragment长度
		uint64 tried_times;	 // 已经尝试下载的次数
		bool downloading;	 // 下载标记
		ptime request_time;	 // 发送请求时间
	};

private:
	bool ApplyDownloadBlock();
	void ResolveBlockToDownloadFragments(const PieceBlock& pb);
	bool IsEqual(const PendingFragment& fragment, const PeerRequest& request);
	void FailToDownloadFragment();
	void ReDownloadFragment(PendingFragment& fragment);
	void SendFragmentDataRequest(const PendingFragment& fragment);
	void DownloadTimeoutProc();
	uint64 GetCurrentSecondDownloadSpeed();
	uint64 GetCurrentSecondUploadSpeed();
	bool IsDownloadSpeedLimited();
	bool IsUploadSpeedLimited();
	void UpdateDownloadSpeed();
	void UpdateUploadSpeed();

private:
	PeerConnection* peer_conn_;

	// 上传/下载队列所有取样的平均下载速度上限
	uint64 download_speed_limit_;	
	uint64 upload_speed_limit_;

	// 当前秒内上传/下载速度限制，用来抑制速率振荡
	uint64 current_download_speed_limit_; 
	uint64 current_upload_speed_limit_;

	// 本端是按照fragment进行下载的
	uint64 fragment_size_;

	// peer发送过来的数据请求
	typedef std::queue<PendingRequest> RequestQueue;
	RequestQueue peer_request_queue_; // 待响应请求 
	boost::mutex peer_request_mutex_;

	// 待下载fragment队列，队列首部是正在下载fragment
	typedef std::queue<PendingFragment> FragmentQueue;
	FragmentQueue download_frag_queue_;
	boost::mutex download_mutex_;

	// 下载速率计算队列，每个元素表示一秒钟内下载的数据大小
	std::queue<uint64> download_speed_queue_;
	uint64 total_download_size_;			// 总下载大小
	uint64 total_download_size_in_queue_;	// 队列中总下载大小
	boost::mutex download_speed_mutex_;

	// 上传速率计算队列，每个元素表示一秒钟内上传的数据大小
	std::queue<uint64> upload_speed_queue_;
	uint64 total_upload_size_;			// 总上传大小
	uint64 total_upload_size_in_queue_;	// 队列中总上传大小
	boost::mutex upload_speed_mutex_;

	//标志block下载失败  TODO:暂时性规避pps重复获取不能下载block
	bool download_block_failed;
};

}
#endif

