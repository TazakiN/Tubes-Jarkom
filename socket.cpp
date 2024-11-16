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
    this->socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (this->socket < 0)
    {
        std::cerr << "Error creating socket" << std::endl;
    }
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
    if (bind(this->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "Error binding socket" << std::endl;
    }

    // Listen on the socket
    if (::listen(this->socket, SOMAXCONN) < 0)
    {
        std::cerr << "Error listening on socket" << std::endl;
    }
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

    // Connect to the destination
    if (connect(this->socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
    {
        std::cerr << "Error connecting to destination" << std::endl;
    }

    // Send data
    ssize_t bytes_sent = ::send(this->socket, dataStream, dataSize, 0);
    if (bytes_sent < 0)
    {
        std::cerr << "Error sending data" << std::endl;
    }
}

// Receive
int32_t TCPSocket::recv(void *buffer, uint32_t length)
{
    ssize_t bytes_received = ::recv(this->socket, buffer, length, 0);
    if (bytes_received < 0)
    {
        std::cerr << "Error receiving data" << std::endl;
    }
    return bytes_received;
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