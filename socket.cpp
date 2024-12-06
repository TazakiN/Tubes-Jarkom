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
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    memset(&(addr.sin_zero), '\0', 8);

    // Bind the socket
    if (::bind(this->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Error binding socket");
    }

    status = TCPStatusEnum::LISTEN;
}

// Send
void TCPSocket::send(std::string ip, int32_t port, void *dataStream, uint32_t dataSize)
{
    // Set up the address structure
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dest_addr.sin_addr);
    memset(&(dest_addr.sin_zero), '\0', 8);

    // Send data
    ssize_t bytes_sent = sendto(this->socket, dataStream, dataSize, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending data" << std::endl;
    }
}

int32_t TCPSocket::recv(void *buffer, uint32_t length)
{
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    ssize_t bytes_received = recvfrom(this->socket, buffer, length, 0, (struct sockaddr *)&src_addr, &addr_len);
    if (bytes_received < 0)
    {
        std::cerr << "Error receiving data" << std::endl;
    }
    return bytes_received;
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