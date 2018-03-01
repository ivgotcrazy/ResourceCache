#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include "bt_msg.hpp"
#include "client_socket.hpp"
#include "bt_msg_recombiner.hpp"
#include "bc_util.hpp"

using namespace BroadCache;

TcpClientSocket* g_sock = nullptr;

BtMsgRecombiner g_msg_recombiner;

static const uint32 kBufSize = 65536;
static char buf[kBufSize];
size_t send_length = 0;

const char* g_info_hash = nullptr;

void set_buffer();
void on_receive(const boost::system::error_code& error, const char* data, size_t bytes_transferred);

void send_handshake();
    
int main(int argc, char* argv[])
{
    boost::cmatch match;
    if ((argc != 4) 
        || !boost::regex_match(argv[1], match, boost::regex("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}"))
        || !boost::regex_match(argv[2], match, boost::regex("\\d{4,5}"))
        || !boost::regex_match(argv[3], match, boost::regex("[A-Z0-9]{40}")))
    {
        std::cout << "usage : <programe> <peer ip> <peer port> <info hash>\n";
        return 1;
    }

    g_info_hash = argv[3];

    boost::asio::io_service ios;
    EndPoint endpoint(to_address_ul(argv[1]), static_cast<unsigned short>(atoi(argv[2])));
    TcpClientSocket sock(ios, endpoint, on_receive); 
    g_sock = &sock;

    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    send_handshake();

    boost::thread io_thread(boost::bind(&boost::asio::io_service::run, &ios)); 
    io_thread.join();
    
    return 0;
}

void send_handshake()
{
    static const char kPeerId[20] = "BI/broadinter";

    HandshakeMsg msg;
    memcpy(msg.info_hash, FromHex(g_info_hash).c_str(), 20);
    memcpy(msg.peer_id, kPeerId, 20);
    msg.extend_supported = true;

    send_length = SetHandshakeMsg(buf, msg);
    g_sock->Send(buf, send_length); 
}

void on_receive(const boost::system::error_code& error, const char* data, size_t bytes_transferred)
{
    if(error || (bytes_transferred == 0) || (data == nullptr))
    {
        return ;
    }

    std::vector<PktMsg> msgs = g_msg_recombiner.GetMsgs(data, bytes_transferred);
    for(const PktMsg& entry : msgs)
    {
        std::cout << std::setw(4) << entry.len << " bytes received : ";
        std::cout << std::setw(15) << msg_type_to_string(GetMsgType(entry.buf, entry.len)) << " msg\n";

/*
        if(IsHandshakeMsg(entry.buf, entry.len))
        {
            const HandshakeMsg* msg = GetHandshakeMsg(entry.buf, entry.len);

            std::cout << "protocol : " << msg->protocol << '\n';
            std::cout << "info-hash : " << msg->info_hash << '\n';
            std::cout << "peer-id : " << msg->peer_id << '\n';
            std::cout << "extend-supported : " << msg->extend_supported << '\n';
        }
*/
    }
}
