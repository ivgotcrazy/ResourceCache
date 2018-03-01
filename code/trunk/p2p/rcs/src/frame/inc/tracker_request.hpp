/*#############################################################################
 * �ļ���   : tracker_request.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��10��29��
 * �ļ����� : TrackerRequest������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef TRACKER_REQUEST_HEAD
#define TRACKER_REQUEST_HEAD

#include "depend.hpp"
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "connection.hpp"

namespace BroadCache
{

/******************************************************************************
 * ����������tracker����Ӧ�ô����������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��10��30��
 *****************************************************************************/
class TrackerRequest : public boost::noncopyable,
	                   public boost::enable_shared_from_this<TrackerRequest>
{
public:
	TrackerRequest(const EndPoint& address) : tracker_address_(address) {}
	virtual ~TrackerRequest();

	void Process();

	void OnTick();

	EndPoint tracker_address() const { return tracker_address_; }

protected:
	EndPoint GetLocalAddress() const;

private:

	// �������ӣ�֧��UDP��TCP
	virtual SocketConnectionSP CreateConnection() = 0;

	// ������������������ƶ��Э�齻�����������ڲ������¼״̬
	virtual bool ConstructSendRequest(SendBuffer& send_buf) = 0;

	// ����ֵ��ʾ�Ƿ���Ҫ�������ͱ��ģ�true: �ǣ�false: ��
	virtual bool ProcessResponse(const char* buf, uint64 len) = 0;

	// ���������ϣ���Լ����Ƴ�ʱ���������ʵ�����������ӿ�
	virtual void TickProc() { return; }
	virtual bool IsTimeout() { return false; }
	
private:
	void OnConnect(const TrackerRequestWP& weak_request, ConnectionError error);
	void OnReceiveData(const TrackerRequestWP& weak_request, ConnectionError error, const char* buf, uint64 len);
	void SendRequest();
	void FinishRequest();

private:
	EndPoint tracker_address_;
	SocketConnectionSP sock_conn_;
};

}

#endif