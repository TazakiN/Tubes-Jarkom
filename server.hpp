#ifndef SERVER_HPP
#define SERVER_HPP

#include "node.hpp"

using namespace std;

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

    string filePath;

public:
    Server(const string &ip, int port);
    void run();
    void handleInputMethod(int mode, bool &retFlag);
    void handleMessage(void *buffer, sockaddr_in *src_addr) override;
};

#endif