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
    string getFilePath();

    string filePath;

public:
    Server(const string &ip, int port);
    void run();
    void handleMessage(void *buffer, sockaddr_in *src_addr) override;
};

#endif