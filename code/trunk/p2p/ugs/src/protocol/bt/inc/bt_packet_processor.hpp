/*##############################################################################
 * �ļ���   : bt_packet_processor.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.23
 * �ļ����� : ���ļ������������bt���ݰ��Ĵ�����
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_BT_PACKET_PROCESSOR
#define HEADER_BT_PACKET_PROCESSOR

#include "bc_typedef.hpp"
#include "bt_types.hpp"
#include "pkt_processor.hpp"

namespace BroadCache
{

/*******************************************************************************
 * ��  ��: bt��http��ʽ��tracker����peer-list���ݰ��Ĵ�����
 * ��  ��: rosan
 * ʱ  ��: 2013.11.23
 ******************************************************************************/
class BtHttpGetPeerProcessor : public PktProcessor
{
public:
    BtHttpGetPeerProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // �������bt���ݰ�
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
    std::list<BtGetPeerRequest> get_peer_requests_;  // peer������tracker����Ϣ
};

/*******************************************************************************
 * ��  ��: bt��udp��ʽ��tracker����peer-list���ݰ��Ĵ�����
 * ��  ��: rosan
 * ʱ  ��: 2013.11.23
 ******************************************************************************/
class BtUdpGetPeerProcessor : public PktProcessor
{
public:
    BtUdpGetPeerProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // �������bt���ݰ�
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
};

/*******************************************************************************
 * ��  ��: bt��dht��ʽ��ȡpeer-list�����ݰ�������
 * ��  ��: rosan
 * ʱ  ��: 2013.11.23
 ******************************************************************************/
class BtDhtGetPeerProcessor : public PktProcessor
{
public:
	BtDhtGetPeerProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // �������bt���ݰ�
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
};

/*******************************************************************************
 * ��  ��: bt����handshake���ݰ��Ĵ�����
 * ��  ��: rosan
 * ʱ  ��: 2013.11.23
 ******************************************************************************/
class BtHandshakeProcessor : public PktProcessor
{
public:
    BtHandshakeProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // �������bt���ݰ�
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
};

}  // namespace BroadCache

#endif  // HEADER_BT_PACKET_PROCESSOR
