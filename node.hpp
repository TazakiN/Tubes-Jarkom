#ifndef node_h
#define node_h

#include "socket.hpp"
#include <netinet/in.h>

/**
 * Abstract class.
 *
 * This is the base class for Server and Client class.
 */
class Node
{
protected:
    TCPSocket *connection;

public:
    void run();
    virtual void handleMessage(void *buffer, struct sockaddr_in *src_addr) = 0;
};

#endif