#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>
#include <string>
// #include <functional>
#include "segment.hpp"
#include "segment_handler.hpp"
#include <netinet/in.h>

using namespace std;

// for references
// https://maxnilz.com/docs/004-network/003-tcp-connection-state/
// You can use a different state enum if you'd like
enum TCPStatusEnum
{
    LISTEN = 0,
    SYN_SENT = 1,
    SYN_RECEIVED = 2,
    ESTABLISHED = 3,
    FIN_WAIT_1 = 4,
    FIN_WAIT_2 = 5,
    CLOSE_WAIT = 6,
    CLOSING = 7,
    LAST_ACK = 8,
    CLOSED = 9
};

class TCPSocket
{
    // todo add tcp connection state?
private:
    /**
     * The ip address and port for the socket instance
     * Not to be confused with ip address and port of the connected connection
     */
    string ip;
    int32_t port;

    /**
     * Socket descriptor
     */
    int32_t socket;

    SegmentHandler *segmentHandler;

    TCPStatusEnum status;

public:
    TCPSocket(const std::string &ip, int32_t port);
    ~TCPSocket();
    std::string getIP();
    int32_t getPort();

    void listen();
    int32_t recvFrom(void *buffer, uint32_t length, sockaddr_in *src_addr);
    void sendTo(sockaddr_in *dest_addr, void *dataStream, uint32_t dataSize);
    void close();

    void setStatus(TCPStatusEnum status);
    void setTimeout(uint32_t milliseconds);
    void unsetTimeout();

    TCPStatusEnum getStatus();

    SegmentHandler *getSegmentHandler();
};

#endif