#ifndef HEADER_PROTOCOL_MSG_MACRO
#define HEADER_PROTOCOL_MSG_MACRO

#include <boost/type_traits/extent.hpp>

namespace BroadCache
{

class PeerConnection;

#define MsgHandlerClass PeerConnection

typedef uint32 MsgId;
typedef bool (MsgHandlerClass::*MsgHandler)(MsgId msg_id, const char* data, uint64 length);
typedef bool (MsgHandlerClass::*DefaultMsgHandler)(MsgId msg_id, const char* data, uint64 length);

struct ProtocolMsgEntry
{
    MsgId msg_id;
    MsgHandler msg_handler;        
};

#define MSG_MAP_BEGIN(class_name, default_msg_handler) \
static const DefaultMsgHandler kDefaultMsgHandler##class_name = \
    static_cast<DefaultMsgHandler>(default_msg_handler); \
static const ProtocolMsgEntry kProtocolMsgEntrys##class_name[] = { \

#define MSG_MAP(msg_id, msg_handler) \
{ static_cast<MsgId>(msg_id), static_cast<MsgHandler>(msg_handler) }, \

#define MSG_MAP_END() \
}; \

#define DISPATCH_MSG(class_name, msg_receiver, message, data, length) \
const decltype(kProtocolMsgEntrys##class_name)& kMsgEntrys \
     = kProtocolMsgEntrys##class_name; \
bool msg_found = false; \
for (size_t i=0; i < boost::extent<decltype(kProtocolMsgEntrys##class_name)>::value; ++i) \
{ \
    if (message == kMsgEntrys[i].msg_id) \
    { \
        (msg_receiver.*(kMsgEntrys[i].msg_handler))(message, data, length); \
        msg_found = true; \
        break; \
    } \
} \
 \
if (!msg_found && (kDefaultMsgHandler##class_name != nullptr)) \
{ \
    (msg_receiver.*(kDefaultMsgHandler##class_name))(message, data, length); \
} \

}

#endif
