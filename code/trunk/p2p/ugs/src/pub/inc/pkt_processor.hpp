/*##############################################################################
 * �ļ���   : pkt_processor.hpp
 * ������   : teck_zhou 
 * ����ʱ�� : 2013��12��24��
 * �ļ����� : ���Ĵ�������
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/

#ifndef HEADER_PKT_PROCESSOR
#define HEADER_PKT_PROCESSOR

#include "bc_typedef.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 * ��  ��: ���ݰ�������Ļ���
 * ��  ��: teck_zhou
 * ʱ  ��: 2013��12��24��
 ******************************************************************************/
class PktProcessor : public boost::noncopyable, 
	                 public boost::enable_shared_from_this<PktProcessor>
{
public:
    virtual ~PktProcessor() {}

	// �ڴ�����β����Ӵ�����
	void AppendProcessor(const PktProcessorSP& successor); 

	// ����������ݰ�
    bool Handle(const void* data, uint32 length);

private:
	// ����������ݰ�������ֵ(true: ���ı�����false:����δ������)
    virtual bool Process(const void* data, uint32 length) = 0;

	void SetSuccessor(const PktProcessorSP& successor) { successor_ = successor; }
	PktProcessorSP GetSuccessor() { return successor_; }

private:
    PktProcessorSP successor_;  // ���α��Ĵ�����
};

}

#endif