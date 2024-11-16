#include "client.hpp"
#include <iostream>
#include "utils.hpp"

Client::Client(const std::string &ip, int port)
{
    this->self_ip = ip;
    this->self_port = port;
    connection = new TCPSocket(ip, port);
}

void Client::run()
{
    printColored("[i] Node is now a receiver", Color::BLUE);
    getServerInfo();

    // TODO: connect ke server
}

void Client::handleMessage(void *buffer)
{
    // Implementasi dari handleMessage()
    // Tambahkan logika penanganan pesan di sini
}

void Client::getServerInfo()
{
    printColored("[?] Please enter the server IP address: ", Color::YELLOW, false);
    std::string ip;
    std::cin >> ip;
    server_ip = ip;

    printColored("[?] Please enter the server port: ", Color::YELLOW, false);
    int port;
    std::cin >> port;
    server_port = port;
}