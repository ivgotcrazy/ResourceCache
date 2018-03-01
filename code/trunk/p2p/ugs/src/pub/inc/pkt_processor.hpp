/*##############################################################################
 * 文件名   : pkt_processor.hpp
 * 创建人   : teck_zhou 
 * 创建时间 : 2013年12月24日
 * 文件描述 : 报文处理公共类
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/

#ifndef HEADER_PKT_PROCESSOR
#define HEADER_PKT_PROCESSOR

#include "bc_typedef.hpp"
#include "ugs_typedef.hpp"

namespace BroadCache
{

/*******************************************************************************
 * 描  述: 数据包处理类的基类
 * 作  者: teck_zhou
 * 时  间: 2013年12月24日
 ******************************************************************************/
class PktProcessor : public boost::noncopyable, 
	                 public boost::enable_shared_from_this<PktProcessor>
{
public:
    virtual ~PktProcessor() {}

	// 在处理链尾部添加处理器
	void AppendProcessor(const PktProcessorSP& successor); 

	// 处理捕获的数据包
    bool Handle(const void* data, uint32 length);

private:
	// 处理捕获的数据包，返回值(true: 报文被处理，false:报文未被处理)
    virtual bool Process(const void* data, uint32 length) = 0;

	void SetSuccessor(const PktProcessorSP& successor) { successor_ = successor; }
	PktProcessorSP GetSuccessor() { return successor_; }

private:
    PktProcessorSP successor_;  // 继任报文处理器
};

}

#endif