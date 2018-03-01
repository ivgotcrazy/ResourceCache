#include <cstring>
#include "bt_msg.hpp"
#include "bt_net_serializer.hpp"

using namespace BroadCache;

//bittorrent�����е�Э���ַ���
static const char* const BIT_TORRENT_PROTOCOL = "BitTorrent protocol";
static const uint8 BIT_TORRENT_PROTOCOL_LEN = 
    static_cast<uint8>(strlen(BIT_TORRENT_PROTOCOL));

size_t SetHandshakeMsg(char* buf, const HandshakeMsg& msg)
{
    char* p = buf;

    memcpy(buf, &BIT_TORRENT_PROTOCOL_LEN, 1);
    buf += 1;
    
    memcpy(buf, BIT_TORRENT_PROTOCOL, BIT_TORRENT_PROTOCOL_LEN);
    buf += BIT_TORRENT_PROTOCOL_LEN;

    char reserved[8];
    memcpy(reserved, msg.reserved, 8);
    if(msg.extend_supported)
    {
        reserved[5] |= 0x10;
    }
    memcpy(buf, reserved, 8);
    buf += 8;

    memcpy(buf, msg.info_hash, 20);
    buf += 20;

    memcpy(buf, msg.peer_id, 20);
    buf += 20;

    return buf - p;
}

size_t SetKeepAliveMsg(char* buf, const KeepAliveMsg& /*msg*/)
{
    BtNetSerializer serializer(buf);
    return serializer.Finish();
}

size_t SetChokeMsg(char* buf, const ChokeMsg& msg)
{
    BtNetSerializer serializer(buf);
    serializer & uint8(msg.choked ? 0 : 1);
    return serializer.Finish();
}

size_t SetInterestedMsg(char* buf, const InterestedMsg& msg)
{
    BtNetSerializer serializer(buf);
    serializer & uint8(msg.interested ? 2 : 3);
    return serializer.Finish();
}

size_t SetHaveMsg(char* buf, const HaveMsg& msg)
{
    BtNetSerializer serializer(buf);
    serializer & uint8(4);
    return serializer.Finish();
}

size_t SetBitfieldMsg(char* buf, const BitfieldMsg& msg)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(5);  // ��Ϣ����

    size_t bits = (7 + msg.bitfield.size()) / 8;  // λ���ֽ���
    char* data = serializer->value();

    memset(data, 0, bits);  // ����������

    for (size_t i = 0; i < msg.bitfield.size(); ++i)
    {
        if(msg.bitfield[i])
        {
            data[i / 8] |= (1 << (7 - i % 8));  // ���ö�Ӧλ
        }
    }

    serializer->advance(bits);

    return serializer.Finish();
}

size_t SetRequestMsg(char* buf, const RequestMsg& msg)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(6)  // ��Ϣ����
        & uint32(msg.index)  // piece index
        & uint32(msg.begin)  // offset in the piece
        & uint32(msg.length);  // block data length

    return serializer.Finish();
}

size_t SetPieceMsg(char* buf, const PieceMsg& msg)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(7) //��Ϣ����
        & uint32(msg.index) //piece index
        & uint32(msg.begin); //block index 
    
    memcpy(serializer->value(), msg.block_data->c_str(), msg.block_data->size()); //block data
    serializer->advance(msg.block_data->size());

    return serializer.Finish();
}

size_t SetCancelMsg(char* buf, const CancelMsg& msg)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(8) //��Ϣ����
        & uint32(msg.index) //piece index
        & uint32(msg.begin) //block index
        & uint32(msg.length); //block data length

    return serializer.Finish();
}

size_t SetPortMsg(char* buf, const PortMsg& msg)
{
    BtNetSerializer serializer(buf);

    serializer & uint8(9) //��Ϣ����
        & uint16(msg.listen_port); //DHT listen port

    return serializer.Finish();
}

bool IsHandshakeMsg(const char* buf, size_t /*len*/)
{
    return (*buf == BIT_TORRENT_PROTOCOL_LEN) 
        && (std::strncmp(BIT_TORRENT_PROTOCOL, buf + 1, BIT_TORRENT_PROTOCOL_LEN) == 0);
}

size_t GetMsgLength(const char* buf, size_t len)
{
    return !IsHandshakeMsg(buf, len) ? 4 + NetToHost<uint32>(buf) 
        : (1 + BIT_TORRENT_PROTOCOL_LEN + 8 + 20 + 20);
}

MsgType GetMsgType(const char* buf, size_t)
{
    //�����ж��Ƿ���keep-alive��Ϣ
    if (NetToHost<uint32>(buf) == 0)
    {
        return MSG_KEEPALIVE;
    }

    //Ȼ���ж��Ƿ���handshake��Ϣ
    if ((NetToHost<uint8>(buf) == BIT_TORRENT_PROTOCOL_LEN) 
        && (std::strncmp(BIT_TORRENT_PROTOCOL, buf + 1, BIT_TORRENT_PROTOCOL_LEN) == 0))
    {
        return MSG_HANDSHAKE;
    }

    //��ȡ��Ϣ����
    MsgType typ = static_cast<MsgType>(NetToHost<uint8>(buf + 4)); 

    //�ж���Ϣ�Ƿ�Ϸ�
    return (((typ >= MSG_BEGIN) && (typ <= MSG_END)) || (typ == MSG_EXTEND)) ? 
        typ : MSG_UNKNOWN; 
}

const HandshakeMsg* GetHandshakeMsg(const char* buf, size_t /*len*/)
{
    static HandshakeMsg msg;

    uint8 protocol_length = 0; //Э���ַ�������
    memcpy(&protocol_length, buf, 1);
    buf += 1;

    msg.protocol.resize(protocol_length); //����Э���ַ���
    memcpy(&msg.protocol[0], buf, protocol_length);
    buf += protocol_length;

    memcpy(msg.reserved, buf, 8);
    buf += 8;

    memcpy(msg.info_hash, buf, 20); //����info-hash
    buf += 20;

    memcpy(msg.peer_id, buf, 20); //����peer id 
    
    msg.extend_supported = ((msg.reserved[5] & 0x10) != 0); //�Ƿ�֧��btЭ����չ

    return &msg;
}

const ChokeMsg* GetChokeMsg(const char* buf, size_t /*len*/)
{
    static ChokeMsg msg;

    uint8 msg_type = 0;
    memcpy(&msg_type, buf + 4, 1);

    msg.choked = (msg_type == 0);

    return &msg;
}

const InterestedMsg* GetInterestedMsg(const char* buf, size_t /*len*/)
{
    static InterestedMsg msg;

    uint8 msg_type = 0;
    memcpy(&msg_type, buf + 4, 1);

    msg.interested = (msg_type == 2);

    return &msg;
}

const HaveMsg* GetHaveMsg(const char* buf, size_t /*len*/)
{
    static HaveMsg msg;

    BtNetUnserializer(buf) & msg.index;

    return &msg;
}

const BitfieldMsg* GetBitfieldMsg(const char* buf, size_t len)
{
    static BitfieldMsg msg;

    msg.bitfield.clear();

    const char* bit_data = buf + 4 + 1; //bitfield���ݿ�ʼλ��
    uint32 bitfield_len = len - 4 - 1; //bitfield���ݳ���
    
    uint32 i = 0;
    while (i < bitfield_len)
    {
        char ch = bit_data[i]; //��ȡ��һλ��Ӧ���ֽ�
        for (int j = 7; j >= 0; --j)
        {
            msg.bitfield.push_back((ch & (1 << j)) != 0);
        }
        ++i;
    }

    return &msg;
}

const RequestMsg* GetRequestMsg(const char* buf, size_t len)
{
    static RequestMsg msg;

    BtNetUnserializer(buf) & msg.index & msg.begin & msg.length;

    return &msg;
} 

const PieceMsg* GetPieceMsg(const char* buf, size_t len)
{
    static PieceMsg msg;

    const char* data = (BtNetUnserializer(buf) & msg.index & msg.begin).value();
    msg.block_data.reset(new std::string(data, buf + len - data));

    return &msg;
}

const CancelMsg* GetCancelMsg(const char* buf, size_t /*len*/)
{
    static CancelMsg msg;

    BtNetUnserializer(buf) & msg.index & msg.begin & msg.length;
    
    return &msg;
}

const PortMsg* GetPortMsg(const char* buf, size_t /*len*/)
{
    static PortMsg msg;

    BtNetUnserializer(buf) & msg.listen_port;
 
    return &msg;
}

const char* msg_type_to_string(MsgType msg_type)
{
    switch(msg_type)
    {
    case MSG_CHOKE : return "choke";
    case MSG_UNCHOKE : return "unchoke";
    case MSG_INTERESTED : return "interested";
    case MSG_NOTINTERESTED : return "not-interested";
    case MSG_HAVE : return "have";
    case MSG_BITFIELD : return "bitfield";
    case MSG_REQUEST : return "request";
    case MSG_PIECE : return "piece";
    case MSG_PORT : return "port";
    case MSG_KEEPALIVE : return "keep-alive";
    case MSG_HANDSHAKE : return "handshake";
    case MSG_EXTEND : return "extend";
    default : return "unknown";
    }
}
