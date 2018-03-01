/*##############################################################################
 * �ļ���   : pkt_processor.hpp
 * ������   : teck_zhou 
 * ����ʱ�� : 2013��12��24��
 * �ļ����� : ���Ĵ�������
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/

#include "pkt_processor.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: �ڴ�����β����Ӵ�����
 * ��  ��: [in] processor ���Ĵ�����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��24��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void PktProcessor::AppendProcessor(const PktProcessorSP& processor)
{
	PktProcessorSP tmp_processor = shared_from_this();
	BC_ASSERT(tmp_processor);

	// ��λ�����������һ��������(û�м�����)
	while (tmp_processor->GetSuccessor())
	{
		tmp_processor = tmp_processor->GetSuccessor();
	}

	tmp_processor->SetSuccessor(processor);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������ݰ�
 * ��  ��: [in] data ����
 *         [in] length ����
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��12��24��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool PktProcessor::Handle(const void* data, uint32 length)
{
	bool processed = Process(data, length); 
	if (!processed && successor_)
	{
		return successor_->Handle(data, length);
	}

	return processed;
}

}