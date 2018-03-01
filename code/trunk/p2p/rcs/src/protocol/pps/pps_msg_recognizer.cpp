/*#############################################################################
 * �ļ���   : pps_msg_recognizer.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��12��30��
 * �ļ����� : BtMsgRecognizerʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_msg_recognizer.hpp"
#include "bc_util.hpp"
#include "bc_io.hpp"
#include "pps_pub.hpp"
#include "net_byte_order.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: Pps��Ϣʶ��
 * ��  ��: [in] buf ��Ϣʶ����
 *         [in] len ��Ϣ������
 *         [out] recognized_len ʶ�������Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��12��30��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PpsCommonMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// Pps��ͨ��Ϣ��ʽ���£�
	// ============================================
	//       2Bytes         5Bytes    
    // |---------------|------------|--------------
    // | length prefix | message ID | payload ...
    // |---------------|------------|--------------
	// <length prefix><message ID><payload>

	uint16 length_prefix = IO::read_int16_be(buf);
						   
	LOG(INFO) << "length_prefix  " <<length_prefix << "len  " << (uint16)len;

	if (length_prefix > (uint16)len) return false;

	buf += 1;
	PpsMsgType type = static_cast<PpsMsgType>(IO::read_uint32(buf));

	LOG(INFO) << " Show message | type: " << type;

	if (type != PPS_MSG_HANDSHAKE || length_prefix == len) //handshake���ͱ��ı�����4,��Ӧ���Ĳ��ü�4�� �ݲ���������д���
	{
		*recognized_len = length_prefix;
	}
	else
	{
		*recognized_len = length_prefix + 4;
	}

	return true;
}

}

