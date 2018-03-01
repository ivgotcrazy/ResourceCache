/*#############################################################################
 * �ļ���   : rcs_util.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : ȫ�ֹ���ʹ�ú���
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "rcs_util.hpp"
#include "disk_io_job.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: ת��PeerRequest
 * ��  ��: [in] read_job ReadDataJob
 * ����ֵ: PeerRequest
 * ��  ��:
 *   ʱ�� 2013��11��04��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
PeerRequest ToPeerRequest(const ReadDataJob& read_job)
{
	PeerRequest request;

	request.piece = read_job.piece;
	request.start = read_job.offset;
	request.length = read_job.len;

	return request;
}

/*------------------------------------------------------------------------------
 * ��  ��: ת��PeerRequest
 * ��  ��: [in] write_job WriteDataJob
 * ����ֵ: PeerRequest
 * ��  ��:
 *   ʱ�� 2013��11��07��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
PeerRequest ToPeerRequest(const WriteDataJob& write_job)
{
	PeerRequest request;

	request.piece = write_job.piece;
	request.start = write_job.offset;
	request.length = write_job.len;

	return request;
}

}
