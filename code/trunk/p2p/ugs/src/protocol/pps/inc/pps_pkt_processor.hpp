/*##############################################################################
 * 文件名   : pps_packet_processor.hpp
 * 创建人   : tom_liu 
 * 创建时间 : 2013.12.24
 * 文件描述 : 此文件包含处理捕获的pps数据包的处理类
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_PPS_PACKET_PROCESSOR
#define HEADER_PPS_PACKET_PROCESSOR

#include "bc_typedef.hpp"
#include "pkt_processor.hpp"
#include "pps_types.hpp"
#include "bc_assert.hpp"

namespace BroadCache
{
/*******************************************************************************
* 描  述: pps请求peer-list数据包的处理类
* 作  者: tom_liu
* 时  间: 2013.12.27
******************************************************************************/
class PpsGetPeerProcessor : public PktProcessor
{
public:
	PpsGetPeerProcessor(const PpsSessionSP session);

private:
    // 处理捕获的bt数据包
    virtual bool Process(const void* data, uint32 length) override;

private:
	void ConstructGetPeerResponse(const char* data, const uint32 length, 
																char* buf, uint32& len);
	
private:
	PpsSessionSP pps_session_;
};

/*******************************************************************************
 * 描  述: pps发送handshake数据包的处理类
 * 作  者: tom_liu
 * 时  间: 2013.12.27
 ******************************************************************************/
class PpsHandshakeProcessor : public PktProcessor
{
public:
	PpsHandshakeProcessor(const PpsSessionSP session);

private:
	// 处理捕获的bt数据包
	virtual bool Process(const void* data, uint32 length) override;

private:
	PpsSessionSP pps_session_;


};

}
#endif
