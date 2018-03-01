#ifndef _MSG_H_
#define _MSG_H_

#include <vector>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include "type_def.hpp"

using namespace BroadCache;

enum MsgType
{
    MSG_BEGIN = 0,
	MSG_CHOKE = MSG_BEGIN,
	MSG_UNCHOKE = 1,
	MSG_INTERESTED = 2,
	MSG_NOTINTERESTED = 3,
	MSG_HAVE = 4,
	MSG_BITFIELD = 5,
	MSG_REQUEST = 6,
	MSG_PIECE = 7,
	MSG_CANCEL = 8,
    MSG_PORT = 9,
    MSG_END = MSG_PORT,

	MSG_KEEPALIVE,
	MSG_HANDSHAKE, 

    MSG_EXTEND = 20,

	MSG_UNKNOWN,
};

const char* msg_type_to_string(MsgType msg_type);

struct HandshakeMsg
{
    std::string protocol; //协议字符串
    char reserved[8]; //保留字段
    char info_hash[20]; //info-hash
    char peer_id[20]; //peer id
    bool extend_supported; //是否支持bt协议扩展
};

struct KeepAliveMsg
{
};

struct ChokeMsg
{
    bool choked;
};

struct InterestedMsg
{
    bool interested;
};

struct HaveMsg
{
    uint32 index;
};

struct BitfieldMsg
{
    boost::dynamic_bitset<> bitfield;
};

struct RequestMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //请求的数据长度
};

struct PieceMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    boost::shared_ptr<std::string> block_data; //块二进制数据
};

struct CancelMsg
{
    uint32 index; //piece index
    uint32 begin; //offset in the piece
    uint32 length; //请求的数据长度
};

struct PortMsg
{
    uint16 listen_port; //告诉接收方节点，发送方支持DHT，并且DHT监听的端口号是listen_port
};

struct MetadataMsg
{
	int type;
	int piece;
	const char* buf;
	int len;
	int total_size;
};

MsgType GetMsgType(const char* buf, size_t len);
size_t GetMsgLength(const char* buf, size_t len);
bool IsHandshakeMsg(const char* buf, size_t len);

const HandshakeMsg* GetHandshakeMsg(const char* buf, size_t len);
const KeepAliveMsg* GetKeepAliveMsg(const char* buf, size_t len);
const ChokeMsg* GetChokeMsg(const char* buf, size_t len);
const InterestedMsg* GetInterestedMsg(const char* buf, size_t len);
const HaveMsg* GetHaveMsg(const char* buf, size_t len);
const BitfieldMsg* GetBitfieldMsg(const char* buf, size_t len);
const RequestMsg* GetRequestMsg(const char* buf, size_t len);
const PieceMsg* GetPieceMsg(const char* buf, size_t len);
const CancelMsg* GetCancelMsg(const char* buf, size_t len);
const PortMsg* GetPortMsg(const char* buf, size_t len);

size_t SetHandshakeMsg(char* buf, const HandshakeMsg& msg);
size_t SetKeepAliveMsg(char* buf, const KeepAliveMsg& msg);
size_t SetChokeMsg(char* buf, const ChokeMsg& msg);
size_t SetInterestedMsg(char* buf, const InterestedMsg& msg);
size_t SetHaveMsg(char* buf, const HaveMsg& msg);
size_t SetBitfieldMsg(char* buf, const BitfieldMsg& msg);
size_t SetRequestMsg(char* buf, const RequestMsg& msg);
size_t SetPieceMsg(char* buf, const PieceMsg& msg);
size_t SetCancelMsg(char* buf, const CancelMsg& msg);
size_t SetPortMsg(char* buf, const PortMsg& msg);

#endif
