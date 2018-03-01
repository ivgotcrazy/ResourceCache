/*##############################################################################
 * �ļ���   : bt_types.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.19
 * �ļ����� : ���ļ�������btЭ��ģ������ͨ�ýṹ������
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_BT_TYPES
#define HEADER_BT_TYPES

#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

static const uint32 kBtGetPeerRequestMax = 1024;  // �����http get peer�����������

// bt���ݰ�����
enum BtPacketType
{
    BT_PACKET_TYPE_BEGIN = 0,  // ��Ҫ�ڴ�ö��֮ǰ�����κ�ö��
    BT_COMMON_PACKET = BT_PACKET_TYPE_BEGIN,
    BT_HTTP_GET_PEER,  // http get��ʽ����peer-list��
    BT_UDP_GET_PEER,  // udp��ʽ����peer-list��
    BT_DHT_GET_PEER,  // ��DHTЭ�鷽ʽ����peer-list��
    BT_HANDSHAKE,  // btЭ���handshake��
    BT_HTTP_GET_PEER_REDIRECT,  // http get����peer-list���Ļظ���
    BT_UDP_GET_PEER_REDIRECT,  // udp��ʽ����peer-list���Ļظ���
    BT_DHT_GET_PEER_REDIRECT,  // dht��ȡpeer-list���Ļظ���
    BT_HANDSHAKE_REDIRECT,  // btЭ��handshake���Ļظ���
    BT_PACKET_TYPE_END  // ���е�ö������֮ǰ���壨��Ҫ�ڴ�ö��֮�����κ�ö�٣�
};

// peer��http get��ʽ��tracker����peer-listʱ��������Ϣ
struct BtGetPeerRequest
{
    std::string info_hash;  // �����ļ���Ӧ��info-hash
    EndPoint requester;  // ���ʹ������peer������ַ
    uint64 file_size = 0;  // peer��Ҫ���ص��ļ���С
    uint16 num_want = 0;  // peer��tracker��������peer����Ŀ
    bool compact = false;
    std::string tracker;  // ���ʹ������Ŀ��tracker������
    std::string event;  // ���ʹ������peer��ǰ�ļ�����״̬
    time_t time = 0;  // peer���ʹ������ʱ��
};

class BtSession;

typedef boost::shared_ptr<BtSession> BtSessionSP;

}  // BroadCache

#endif  // HEADER_BT_TYPES
