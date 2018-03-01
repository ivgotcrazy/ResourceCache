/*#############################################################################
 * �ļ���   : pkt_capturer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : SessionManager������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PKT_CAPTURER
#define HEADER_PKT_CAPTURER

#include <string>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include "pcap.h"

#include "bc_typedef.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * ���������Ĳ�����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013��11��13��
 *****************************************************************************/
class PktCapturer : public boost::noncopyable
{
public:
	typedef boost::function<void(const Packet&)> PktHandler;

	PktCapturer(const std::string& filter, PktHandler handler);
	~PktCapturer();

	bool Init();
	bool Start();
	bool Stop();

private:
	void PktCaptureFunc(pcap_t* session);
	bool OpenCaptureInterface();
	bool SetCaptureFilter();

private:
	std::string pkt_filter_;
	PktHandler pkt_handler_;

	// ÿ������ʹ��һ���߳�ץ��
	std::vector<boost::thread> capture_threads_;

	// ÿ��������Ӧһ���Ự
	std::vector<pcap_t*> capture_sessions_;

	bool stop_flag_;
};

}

#endif
