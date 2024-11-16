#include <iostream>
#include <string>
#include "server.hpp"
#include "client.hpp"
#include "utils.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    // handle salah input
    if (argc != 3)
    {
        printColored("Usage: " + string(argv[0]) + " <ip> <port>", Color::RED);
        return 1;
    }

    string ip = argv[1];
    int port = stoi(argv[2]);
    printColored("[i] Node started at " + ip + ":" + to_string(port), Color::BLUE);

    printColored("[?] Please enter the operating mode (sender / receiver):", Color::YELLOW);
    printColored("[?] 1. Sender", Color::YELLOW);
    printColored("[?] 2. Receiver", Color::YELLOW);
    printColored("[?] Enter your choice: ", Color::YELLOW, false);
    int choice;
    cin >> choice;

    if (choice == 1)
    {
        printColored("[i] Node is now a sender", Color::BLUE);
        Server server(ip, port);
        server.run();
    }
    else
    {
        printColored("[i] Node is now a receiver", Color::BLUE);
        Client client(ip, port);
        client.run();
    }

    return 0;
}