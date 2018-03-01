/*#############################################################################
 * 文件名   : pkt_capturer.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : SessionManager类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * 描述：报文捕获器
 * 作者：teck_zhou
 * 时间：2013年11月13日
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

	// 每个网卡使用一个线程抓包
	std::vector<boost::thread> capture_threads_;

	// 每个网卡对应一个会话
	std::vector<pcap_t*> capture_sessions_;

	bool stop_flag_;
};

}

#endif
