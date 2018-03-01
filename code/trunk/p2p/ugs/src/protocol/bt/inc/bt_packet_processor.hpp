/*##############################################################################
 * 文件名   : bt_packet_processor.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.11.23
 * 文件描述 : 此文件包含处理捕获的bt数据包的处理类
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_BT_PACKET_PROCESSOR
#define HEADER_BT_PACKET_PROCESSOR

#include "bc_typedef.hpp"
#include "bt_types.hpp"
#include "pkt_processor.hpp"

namespace BroadCache
{

/*******************************************************************************
 * 描  述: bt以http方式向tracker请求peer-list数据包的处理类
 * 作  者: rosan
 * 时  间: 2013.11.23
 ******************************************************************************/
class BtHttpGetPeerProcessor : public PktProcessor
{
public:
    BtHttpGetPeerProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // 处理捕获的bt数据包
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
    std::list<BtGetPeerRequest> get_peer_requests_;  // peer的请求tracker的信息
};

/*******************************************************************************
 * 描  述: bt以udp方式向tracker请求peer-list数据包的处理类
 * 作  者: rosan
 * 时  间: 2013.11.23
 ******************************************************************************/
class BtUdpGetPeerProcessor : public PktProcessor
{
public:
    BtUdpGetPeerProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // 处理捕获的bt数据包
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
};

/*******************************************************************************
 * 描  述: bt以dht方式获取peer-list的数据包处理类
 * 作  者: rosan
 * 时  间: 2013.11.23
 ******************************************************************************/
class BtDhtGetPeerProcessor : public PktProcessor
{
public:
	BtDhtGetPeerProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // 处理捕获的bt数据包
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
};

/*******************************************************************************
 * 描  述: bt发送handshake数据包的处理类
 * 作  者: rosan
 * 时  间: 2013.11.23
 ******************************************************************************/
class BtHandshakeProcessor : public PktProcessor
{
public:
    BtHandshakeProcessor(BtSessionSP session) : bt_session_(session) {}

private:
    // 处理捕获的bt数据包
    virtual bool Process(const void* data, uint32 length) override;

private:
	BtSessionSP bt_session_;
};

}  // namespace BroadCache

#endif  // HEADER_BT_PACKET_PROCESSOR
