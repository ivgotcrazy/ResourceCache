/*#############################################################################
 * �ļ���   : pkt_capturer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : SessionManager������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pkt_capturer.hpp"
#include "bc_assert.hpp"
#include "ugs_util.hpp"
#include "mem_buf_pool.hpp"
#include "ugs_config_parser.hpp"

namespace BroadCache
{

static const int kMaxPacketSize = 1518;
static const int kMinPacketSize = 42;

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PktCapturer::PktCapturer(const std::string& filter, PktHandler handler)
	: pkt_filter_(filter), pkt_handler_(handler), stop_flag_(false)
{
	BC_ASSERT(!pkt_filter_.empty());
	BC_ASSERT(pkt_handler_);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��������
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
PktCapturer::~PktCapturer()
{
	stop_flag_ = true;

	for (boost::thread& thread : capture_threads_)
	{
		thread.join();
	}

	for (pcap_t* session : capture_sessions_)
	{
		pcap_close(session);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �򿪲�������
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PktCapturer::OpenCaptureInterface()
{
	// �������ļ���ȡץ�������б�
	std::string interface_str;
    GET_UGS_CONFIG_STR("global.common.capture-interface", interface_str);	

	if (interface_str.empty())
	{
		LOG(ERROR) << "Empty capture interface";
		return false;
	}

	std::vector<std::string> capture_interfaces = SplitStr(interface_str, ' ');
	BC_ASSERT(!capture_interfaces.empty());

	// ��ץ������

	char errbuf[PCAP_ERRBUF_SIZE];
	for (std::string& capture_interface : capture_interfaces)
	{
		pcap_t* session = pcap_open_live(capture_interface.c_str(),	// name of the device
										 kMaxPacketSize,			// portion of the packet to capture
										 1,							// promiscuous mode (nonzero means promiscuous)
										 1,							// read timeout
										 errbuf);					// error buffer
		if (!session)
		{
			LOG(ERROR) << "Fail to open interface | " << capture_interface;
			return false;
		}

		capture_sessions_.push_back(session);

        LOG(INFO) << "Open capture interface " << capture_interface;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ò������
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PktCapturer::SetCaptureFilter()
{
	pcap_direction_t direction = PCAP_D_IN; // �̶�ֻץ�뷽��ı���

	for (pcap_t* session : capture_sessions_)
	{
		if (-1 == pcap_setdirection(session, direction))
		{
			LOG(ERROR) << "Fail to capture direction | " << pcap_geterr(session);
			return false;
		}

		int datalink_type = pcap_datalink(session);
		if (-1 == datalink_type)
		{
			LOG(ERROR) << "Fail to get datalink type | " << pcap_geterr(session);
			return false;
		}

		// ������˹���
		bpf_program compiled_filter;
		if (-1 == pcap_compile_nopcap(kMaxPacketSize, 
									  datalink_type, 
									  &compiled_filter, 
									  pkt_filter_.c_str(), 
									  1, 
									  0xFFFFFFFF))
		{
			LOG(ERROR) << "Fail to compile pcap filter | " << pcap_geterr(session);
			return false;
		}

		// ���ù��˹���
		if (-1 == pcap_setfilter(session, &compiled_filter))
		{
			LOG(ERROR) << "Fail to set capture filter " << pcap_geterr(session);
			return false;
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PktCapturer::Init()
{
	if (!OpenCaptureInterface())
	{
		LOG(ERROR) << "Fail to open capture interface";
		return false;
	}

	if (!SetCaptureFilter())
	{
		LOG(ERROR) << "Fail to set capture filter";
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ץ��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PktCapturer::Start()
{
	for (pcap_t* session : capture_sessions_)
	{
		capture_threads_.push_back(boost::thread(
			boost::bind(&PktCapturer::PktCaptureFunc, this, session)));
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣץ��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��13��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PktCapturer::Stop()
{
	stop_flag_ = true;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���Ĳ����̴߳�����
 * ��  ��: [in] session ץ������
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PktCapturer::PktCaptureFunc(pcap_t* session)
{
	int ret;
	pcap_pkthdr* header = nullptr;
	const u_char* packet = nullptr;
	Packet copy_packet;

	while (!stop_flag_)
	{
		ret = pcap_next_ex(session, &header, &packet);
		if (ret < 0)
		{
			LOG(ERROR) << "Fail to read packets | " << pcap_geterr(session);
			return;
		}

		// Timeout elapsed
		if (ret == 0) continue; 

		// that means the packet has been truncated or it's a malformed packet
		if (header->caplen != header->len || header->len < kMinPacketSize)
		{
			continue;
		}

		// ����������һ�ο��������ܻ�Ӱ�����ܣ������ѱ��Ĵ������������
		// ���̣߳���ν�е���ʧ����������ܵ�Ӱ�컹��Ҫʵ�ʲ�����֤��
		char* temp = MemBufPool::GetInstance().AllocPktBuf();
		if (!temp) continue;

		std::memcpy(temp, packet, header->len);

		copy_packet.buf = temp;
		copy_packet.len = header->len;

		pkt_handler_(copy_packet);
	}
}

}
