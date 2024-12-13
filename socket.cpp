// socket.cpp

#include "socket.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

// Constructor
TCPSocket::TCPSocket(const std::string &ip, int32_t port) : ip(ip), port(port), socket(-1)
{
    // Create socket
    this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (this->socket < 0)
    {
        std::cerr << "Error creating socket" << std::endl;
    }

    segmentHandler = new SegmentHandler();
}

// Destructor
TCPSocket::~TCPSocket()
{
    close();
}

// Get IP
std::string TCPSocket::getIP()
{
    return ip;
}

// Get Port
int32_t TCPSocket::getPort()
{
    return port;
}

// Listen
void TCPSocket::listen()
{

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(addr.sin_zero), '\0', 8);

    // Bind the socket
    if (::bind(this->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Error binding socket");
    }

    status = TCPStatusEnum::LISTEN;
}

int32_t TCPSocket::recvFrom(void *buffer, uint32_t length, struct sockaddr_in *src_addr)
{
    socklen_t addr_len = sizeof(struct sockaddr_in);
    ssize_t bytes_received = recvfrom(this->socket, buffer, length, 0, (struct sockaddr *)src_addr, &addr_len);
    if (bytes_received < 0)
    {
        std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
    }
    return bytes_received;
}

void TCPSocket::sendTo(struct sockaddr_in *dest_addr, void *dataStream, uint32_t dataSize)
{
    int broadcastEnable = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        perror("Error setting broadcast permission");
        exit(1);
    }

    ssize_t bytes_sent = sendto(this->socket, dataStream, dataSize, 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr));
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending data: " << strerror(errno) << std::endl;
    }
}

// Close
void TCPSocket::close()
{
    if (this->socket >= 0)
    {
        ::close(this->socket);
        this->socket = -1;
    }
}

void TCPSocket::setStatus(TCPStatusEnum status)
{
    this->status = status;
}

TCPStatusEnum TCPSocket::getStatus()
{
    return status;
}

SegmentHandler *TCPSocket::getSegmentHandler()
{
    return segmentHandler;
}

void TCPSocket::setTimeout(uint32_t milliseconds)
{
    struct timeval timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;

    if (setsockopt(this->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
    }
}

void TCPSocket::unsetTimeout()
{
    struct timeval timeout = {0, 0};

    if (setsockopt(this->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        std::cerr << "Error unsetting receive timeout: " << strerror(errno) << std::endl;
    }

    if (setsockopt(this->socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        std::cerr << "Error unsetting send timeout: " << strerror(errno) << std::endl;
    }
}
