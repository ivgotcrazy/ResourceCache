/*#############################################################################
 * �ļ���   : bt_msg_recognizer.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��19��
 * �ļ����� : BtMsgRecognizerʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bt_msg_recognizer.hpp"
#include "bc_util.hpp"
#include "bt_pub.hpp"
#include "bc_io.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: BT������Ϣʶ��
 * ��  ��: [in] buf ��Ϣʶ����
 *         [in] len ��Ϣ������
 *         [out] recognized_len ʶ�������Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtHandshakeMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// BT������Ϣ��ʽ���£�
	// ====================================================================
	//    1Byte          19Bytes         8Bytes     20Bytes      20Bytes
    // |--------|---------------------|----------|-----------------------|
    // |   19   | BitTorrent Protocol | reserved | info_hash  | peer_id  |
    // |--------|---------------------|----------|-----------------------|
	// <pstrlen><pstr><reserved><info_hash><peer_id>

	if (len < BT_HANDSHAKE_PKT_LEN) return false;

	if (*buf != BT_HANDSHAKE_PROTOCOL_STR_LEN) return false;

	if (std::strncmp(BT_HANDSHAKE_PROTOCOL_STR, buf + 1, BT_HANDSHAKE_PROTOCOL_STR_LEN) != 0)
	{
		return false;
	}

	*recognized_len = BT_HANDSHAKE_PKT_LEN;

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: BT��ͨ��Ϣʶ��
 * ��  ��: [in] buf ��Ϣʶ����
 *         [in] len ��Ϣ������
 *         [out] recognized_len ʶ�������Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BtCommonMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// BT��ͨ��Ϣ��ʽ���£�
	// ============================================
	//       4Bytes         1Byte    
    // |---------------|------------|--------------
    // | length prefix | message ID | payload ...
    // |---------------|------------|--------------
	// <length prefix><message ID><payload>

	uint32 length_prefix = IO::read_uint32(buf);

	if (length_prefix + 4 > len) return false;

	*recognized_len = length_prefix + 4;

	return true;
}

}
