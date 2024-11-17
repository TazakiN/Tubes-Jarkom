#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "node.hpp"
#include <vector>

class Client : public Node
{
private:
    string server_ip;
    int server_port;

    string self_ip;
    int self_port;

    void getServerInfo();

    void sendSYN();
    void sendACK(Segment *segment);

public:
    Client(const std::string &ip, int port);
    void run();
    void handleMessage(void *buffer, struct sockaddr_in *client_addr) override;
};

#endif