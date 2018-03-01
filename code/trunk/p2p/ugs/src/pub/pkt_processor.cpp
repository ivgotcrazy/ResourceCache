/*##############################################################################
 * 文件名   : pkt_processor.hpp
 * 创建人   : teck_zhou 
 * 创建时间 : 2013年12月24日
 * 文件描述 : 报文处理公共类
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/

#include "pkt_processor.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 在处理链尾部添加处理器
 * 参  数: [in] processor 报文处理器
 * 返回值: 
 * 修  改:
 *   时间 2013年12月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void PktProcessor::AppendProcessor(const PktProcessorSP& processor)
{
	PktProcessorSP tmp_processor = shared_from_this();
	BC_ASSERT(tmp_processor);

	// 定位处理链的最后一个处理器(没有继任者)
	while (tmp_processor->GetSuccessor())
	{
		tmp_processor = tmp_processor->GetSuccessor();
	}

	tmp_processor->SetSuccessor(processor);
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理捕获的数据包
 * 参  数: [in] data 数据
 *         [in] length 长度
 * 返回值: 
 * 修  改:
 *   时间 2013年12月24日
 *   作者 teck_zhou
 *   描述 创建
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