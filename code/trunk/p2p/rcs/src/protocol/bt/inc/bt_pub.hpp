/*#############################################################################
 * �ļ���   : bt_pub.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��20��
 * �ļ����� : BTЭ���ڲ����úꡢ�����������ȶ���
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BT_PUB
#define HEADER_BT_PUB

namespace BroadCache
{

#define BT_FAST_RESUME_VERSION	"0.1.0"			// fast-resume�汾
#define BT_FAST_RESUME_DESC		"fast-resume"	// fast-resume����
#define BT_FAST_RESUME_PROTOCOL "bt"			// fast-resumeЭ������

#define BT_INFO_HASH_LEN	20

// BT������Ϣ��ʽ���£�
// ====================================================================
//    1Byte          19Bytes         8Bytes     20Bytes      20Bytes
// |--------|---------------------|----------|-----------------------|
// |   19   | BitTorrent Protocol | reserved | info_hash  | peer_id  |
// |--------|---------------------|----------|-----------------------|
// <pstrlen><pstr><reserved><info_hash><peer_id>
#define BT_HANDSHAKE_PROTOCOL_STR		"BitTorrent protocol"
#define BT_HANDSHAKE_PROTOCOL_STR_LEN	19
#define BT_HANDSHAKE_PKT_LEN			(1 + 19 + 8 + 20 + 20)

}

#endif