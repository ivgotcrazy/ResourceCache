/*#############################################################################
 * �ļ���   : distri_download_msg_recognizer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��19��
 * �ļ����� : DistriDownloadMsgRecognizerʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include <glog/logging.h>

#include "distri_download_msg_recognizer.hpp"
#include "bc_util.hpp"
#include "bc_io.hpp"
#include "rcs_config.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ˽��Э����Ϣʶ��
 * ��  ��: [in] buf ��Ϣʶ����
 *         [in] len ��Ϣ������
 *         [out] recognized_len ʶ�������Ϣ����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��19��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool DistriDownloadMsgRecognizer::RecognizeMsg(const char* buf, uint32 len, uint32* recognized_len)
{
	// ˽�б��ĸ�ʽ���£�
	// ====================================================================
	//     			4Bytes                    2Bytes        1Byte
    // |--------------------------------|----------------|--------|--------
    // |         magic-number           |     length     |  type  | data...
    // |--------------------------------|----------------|--------|--------

	if (len < 4 + 2 + 1) return false;

	const char* tmp = buf;

	uint32 magic_number = IO::read_uint32(tmp);

	// ƥ��ħ����
	if (DISTRIBUTED_DOWNLOAD_MAGIC_NUMBER != magic_number) return false;

	uint32 pkt_len = IO::read_uint16(tmp);
	
	// У�鳤���ֶΣ�����������Ӧ��ҪУ��ͨ��
	if (pkt_len + 4 + 2 > len) 
	{
		LOG(WARNING) << "Fail to check length for private msg | pkt_len: " 
			         << pkt_len << " len: " << len;
		return false;
	}

	*recognized_len = pkt_len + 4 + 2;

	return true;
}

}