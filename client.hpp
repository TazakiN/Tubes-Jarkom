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
    void handleFileData(Segment *segment);
    void sendACK(Segment *segment);

    std::vector<uint8_t> receivedData;
    uint32_t initialSeqNum;
    uint32_t lastAckedSeqNum;

    size_t totalDataSize;

    int received_seg;

    void handleFileTransferFin();

public:
    Client(const std::string &ip, int port);
    void run();
    void handleMessage(void *buffer, struct sockaddr_in *client_addr) override;
};

#endif