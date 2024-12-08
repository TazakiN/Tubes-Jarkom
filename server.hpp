#ifndef SERVER_HPP
#define SERVER_HPP

#include "node.hpp"
#include <unistd.h>
#include <unordered_map>
#include <thread>
#include <mutex>

using namespace std;

const int TIMEOUT_INTERVAL = 100;

class Server : public Node
{

private:
    int inputMethodMenu();
    string getUserInput();
    string saveToFile(const string &data);
    void initTransfer(Segment *segment, sockaddr_in *client_addr);
    void sendNextWindow(sockaddr_in *client_addr);
    void handleFileTransferAck(Segment *segment, sockaddr_in *client_addr);
    void handleFINACK(Segment *segment, sockaddr_in *client_addr);
    string getFilePath();
    void startTimer(Segment segment, struct sockaddr_in *client_addr);

    string filePath;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    // sliding window
    uint32_t SWS;
    uint32_t LAR;
    uint32_t LFS;
    uint32_t initialSeqNum;
    uint32_t handshakeSeqNum;
    std::unordered_map<uint32_t, std::thread> timers;
    std::mutex timerMutex;

public:
    Server(const string &ip, int port);
    void run();
    void handleInputMethod(int mode, bool &retFlag);
    void handleMessage(void *buffer, sockaddr_in *src_addr) override;
};

#endif