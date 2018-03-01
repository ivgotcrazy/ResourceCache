/*##############################################################################
 * �ļ���   : pps_packet_processor.hpp
 * ������   : tom_liu 
 * ����ʱ�� : 2013.12.24
 * �ļ����� : ���ļ������������pps���ݰ��Ĵ�����
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_PPS_PACKET_PROCESSOR
#define HEADER_PPS_PACKET_PROCESSOR

#include "bc_typedef.hpp"
#include "pkt_processor.hpp"
#include "pps_types.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{
/*******************************************************************************
* ��  ��: pps����peer-list���ݰ��Ĵ�����
* ��  ��: tom_liu
* ʱ  ��: 2013.12.27
******************************************************************************/
class PpsGetPeerProcessor : public PktProcessor
{
public:
	PpsGetPeerProcessor(const PpsSessionSP session);

private:
    // �������bt���ݰ�
    virtual bool Process(const void* data, uint32 length) override;

private:
	void ConstructGetPeerResponse(const char* data, const uint32 length, 
																char* buf, uint32& len);
	
private:
	PpsSessionSP pps_session_;
};

/*******************************************************************************
 * ��  ��: pps����handshake���ݰ��Ĵ�����
 * ��  ��: tom_liu
 * ʱ  ��: 2013.12.27
 ******************************************************************************/
class PpsHandshakeProcessor : public PktProcessor
{
public:
	PpsHandshakeProcessor(const PpsSessionSP session);

private:
	// �������bt���ݰ�
	virtual bool Process(const void* data, uint32 length) override;

private:
	PpsSessionSP pps_session_;


};

}
#endif
