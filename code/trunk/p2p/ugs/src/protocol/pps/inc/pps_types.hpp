/*##############################################################################
 * �ļ���   : pps_types.hpp
 * ������   : tom_liu 
 * ����ʱ�� : 2013.12.24
 * �ļ����� : ���ļ�������ppsЭ��ģ������ͨ�ýṹ������
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_PPS_TYPES
#define HEADER_PPS_TYPES

#include "bc_typedef.hpp"
#include "endpoint.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

static const uint32 kPpsGetPeerRequestMax = 1024;  // �����http get peer�����������

// bt���ݰ�����
enum PpsPacketType
{
    PPS_PACKET_TYPE_BEGIN = 0,  // ��Ҫ�ڴ�ö��֮ǰ�����κ�ö��
    PPS_COMMON_PACKET = PPS_PACKET_TYPE_BEGIN,
    PPS_GET_PEER,  // ����peer-list��
    PPS_HANDSHAKE,  // ppsЭ���handshake��
    PPS_GET_META_INFO, //����meta��Ϣ
    PPS_GET_BASE_INFO, //����base��Ϣ
    PPS_PACKET_TYPE_END  // ���е�ö������֮ǰ���壨��Ҫ�ڴ�ö��֮�����κ�ö�٣�
};

// pps���ݰ����Ͷ�Ӧ���ַ�����Ϣ
const char* const kPpsPktTypeDescriptions[PPS_PACKET_TYPE_END] = 
{
	"common",
	"Get peer",
	"Pps handshake",
	"Get meta info",
	"Get base info"
};

class PpsSession;

typedef boost::shared_ptr<PpsSession> PpsSessionSP;

}  // BroadCache

#endif  // HEADER_BT_TYPES
