#include <gtest/gtest.h>
#include "pub/inc/client_socket.hpp"
#include "pub/inc/server_socket.hpp"

namespace BroadCache
{

static const std::size_t DATA_LEN = 16;
static const char DATA[DATA_LEN] = "hello world";
static const short PORT = 1234;

boost::asio::io_service io_service;

namespace tcp_client_send
{

const char* server_received_data = NULL;
std::size_t server_received_length = 0;
bool server_received = false;

void client_receive(bool ok, const char* data, std::size_t length)
{
}

void server_receive(const EndPoint& endpoint, bool ok, const char* data, std::size_t length)
{
    if(ok)
    {
        server_received_data = data;
        server_received_length = length; 
        server_received = true;
    }
}

TEST(tcp_client, send)
{
    TcpServerSocket tcp_server(PORT, io_service, server_receive);
    TcpClientSocket tcp_client(io_service, 
        EndPoint(boost::asio::ip::address_v4::loopback().to_ulong(), PORT),
        client_receive);

    while(!tcp_client.IsConnected()) {}

    tcp_client.Send(DATA, DATA_LEN); 

    while(!server_received) {}

    ASSERT_EQ(server_received_length, DATA_LEN);
    EXPECT_EQ(std::memcmp(DATA, server_received_data, DATA_LEN), 0);
}
}

namespace tcp_client_receive
{

const char* client_received_data = NULL;
std::size_t client_received_length = 0;
bool client_received = false;
TcpServerSocket* ptcp_server = 0;

void client_receive(bool ok, const char* data, std::size_t length)
{
    if(ok)
    {
        client_received_data = data;
        client_received_length = length;
        client_received = true;
    }
}

void server_receive(const EndPoint& endpoint, bool ok, const char* data, std::size_t length)
{
    if(ok)
    {
        ptcp_server->Send(endpoint, DATA, DATA_LEN);
    }
}

TEST(tcp_client, receive)
{
    TcpServerSocket tcp_server(PORT, io_service, server_receive);
    TcpClientSocket tcp_client(io_service, 
        EndPoint(boost::asio::ip::address_v4::loopback().to_ulong(), PORT),
        client_receive);

    ptcp_server = &tcp_server;

    while(!tcp_client.IsConnected()) {}

    tcp_client.Send(DATA, DATA_LEN); 

    while(!client_received) {}

    ASSERT_EQ(client_received_length, DATA_LEN);
    EXPECT_EQ(std::memcmp(DATA, client_received_data, DATA_LEN), 0);
}

}
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv); 
    int ret = RUN_ALL_TESTS();
    BroadCache::io_service.run();

    return ret;
}
