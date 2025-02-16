#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "node.hpp"
#include <vector>
#include <unordered_map>
#include <unistd.h>

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
    std::vector<uint8_t> receivedIndex;
    uint32_t initialSeqNum;
    uint32_t lastAckedSeqNum;

    size_t totalDataSize;

    int received_seg;

    std::string filename;
    bool filenameReceived;

    void handleFileTransferFin();
    void sendFileACK(Segment *segment);
    bool isContiguous(uint32_t start, uint32_t end);

    sockaddr_in server_addr;

    // sliding window
    uint32_t RWS;
    uint32_t LFR;
    uint32_t LAF;
    uint32_t SeqNumACK;
    std::unordered_map<uint32_t, Segment> buffer;
    
    //CRC
    unsigned int CRC: 1;

public:
    Client(const std::string &ip, int port);
    void run();
    void handleMessage(void *buffer, struct sockaddr_in *client_addr) override;
};

#endif