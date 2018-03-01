/*#############################################################################
 * �ļ���   : traffic_controller.hpp
 * ������   : teck_zhou
 * ����ʱ�� : 2013��10��13��
 * �ļ����� : TrafficController������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ����: ���Ӵ������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/12
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
		uint64 frag_index;   // fragment����
		uint64 block_offset; // block���ֽ�ƫ��
		uint64 frag_len;     // fragment����
		uint64 tried_times;	 // �Ѿ��������صĴ���
		bool downloading;	 // ���ر��
		ptime request_time;	 // ��������ʱ��
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

	// �ϴ�/���ض�������ȡ����ƽ�������ٶ�����
	uint64 download_speed_limit_;	
	uint64 upload_speed_limit_;

	// ��ǰ�����ϴ�/�����ٶ����ƣ���������������
	uint64 current_download_speed_limit_; 
	uint64 current_upload_speed_limit_;

	// �����ǰ���fragment�������ص�
	uint64 fragment_size_;

	// peer���͹�������������
	typedef std::queue<PendingRequest> RequestQueue;
	RequestQueue peer_request_queue_; // ����Ӧ���� 
	boost::mutex peer_request_mutex_;

	// ������fragment���У������ײ�����������fragment
	typedef std::queue<PendingFragment> FragmentQueue;
	FragmentQueue download_frag_queue_;
	boost::mutex download_mutex_;

	// �������ʼ�����У�ÿ��Ԫ�ر�ʾһ���������ص����ݴ�С
	std::queue<uint64> download_speed_queue_;
	uint64 total_download_size_;			// �����ش�С
	uint64 total_download_size_in_queue_;	// �����������ش�С
	boost::mutex download_speed_mutex_;

	// �ϴ����ʼ�����У�ÿ��Ԫ�ر�ʾһ�������ϴ������ݴ�С
	std::queue<uint64> upload_speed_queue_;
	uint64 total_upload_size_;			// ���ϴ���С
	uint64 total_upload_size_in_queue_;	// ���������ϴ���С
	boost::mutex upload_speed_mutex_;

	//��־block����ʧ��  TODO:��ʱ�Թ��pps�ظ���ȡ��������block
	bool download_block_failed;
};

}
#endif

