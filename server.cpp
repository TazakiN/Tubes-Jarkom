#include "server.hpp"
#include <iostream>
#include <fstream>
#include "utils.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

Server::Server(const string &ip, int port)
{
    connection = new TCPSocket(ip, port);
}

void Server::run()
{
    printColored("[i] Node is now a sender", Color::BLUE);
    int mode = inputMethodMenu();

    if (mode == 1)
    {
        string userInput = getUserInput(); // Input manual disimpan ke file
        if (!userInput.empty())
        {
            filePath = saveToFile(userInput);
            if (!filePath.empty())
            {
                printColored("[+] User input saved to file.", Color::GREEN);
            }
        }
        else
        {
            printColored("[!] No input provided. Exiting...", Color::RED);
            return;
        }
    }
    else if (mode == 2) // File eksternal
    {
        filePath = getFilePath();
        if (filePath.empty())
        {
            printColored("[!] Invalid file path. Exiting...", Color::RED);
            return;
        }
    }

    // Verify file exists and is readable
    if (!std::ifstream(filePath).good())
    {
        printColored("[!] Cannot access file: " + filePath, Color::RED);
        return;
    }

    connection->listen();
    printColored("[i] Listening to the broadcast port for clients.", Color::BLUE);
    while (true)
    {
        Segment receivedSegment;
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int32_t bytes_received = connection->recvFrom(&receivedSegment, sizeof(receivedSegment), &client_addr);
        if (bytes_received > 0)
        {
            if (receivedSegment.flags == SYN_FLAG)
            {
                std::thread(&Server::handleMessage, this, &receivedSegment, &client_addr).detach();
            }
            else
            {
                handleMessage(&receivedSegment, &client_addr);
            }
        }
    }
}

void Server::handleMessage(void *buffer, struct sockaddr_in *client_addr)
{
    Segment *segment = (Segment *)buffer;
    // Dapatkan IP dan port klien
    string client_ip = inet_ntoa(client_addr->sin_addr);
    int client_port = ntohs(client_addr->sin_port);

    if (segment->flags == SYN_FLAG)
    {
        // Penanganan SYN seperti sebelumnya
        printColored("[+] [Handshake] [S=" + to_string(segment->seqNum) + "] Received SYN from " + client_ip + ":" + to_string(client_port), Color::GREEN);

        // Membuat segmen SYN-ACK
        Segment synAckSegment = synAck(segment->seqNum);
        synAckSegment.ackNum = segment->seqNum + 1;

        // Kirim segmen SYN-ACK ke klien
        connection->sendTo(client_addr, &synAckSegment, sizeof(synAckSegment));

        printColored("[+] [Handshake] [A=" + to_string(synAckSegment.ackNum) + "] [S=" + to_string(synAckSegment.seqNum) + "] Sent SYN-ACK to " + client_ip + ":" + to_string(client_port), Color::GREEN);

        // Perbarui nomor sequence server
        uint32_t server_seq_num = synAckSegment.ackNum;
        connection->setStatus(TCPStatusEnum::SYN_RECEIVED);
    }
    else if (segment->flags == ACK_FLAG)
    {
        printColored("[+] [Handshake] [A=" + to_string(segment->ackNum) + "] Received ACK from " + client_ip + ":" + to_string(client_port), Color::GREEN);
        connection->setStatus(TCPStatusEnum::ESTABLISHED);
        printColored("[~] Connection established with " + client_ip + ":" + to_string(client_port), Color::PURPLE);
        printColored("[+] Sending file to " + client_ip + ":" + to_string(client_port), Color::GREEN);

        // TODO: Implement file sending
    }
    else
    {
        printColored("[!] Received unexpected segment", Color::RED);
    }
}

int Server::inputMethodMenu()
{
    printColored("[?] Please choose the sending mode:", Color::YELLOW);
    printColored("[?] 1. User Input", Color::YELLOW);
    printColored("[?] 2. File Input", Color::YELLOW);
    printColored("[?] Enter your choice: ", Color::YELLOW, false);

    int choice;
    cin >> choice;
    cin.ignore();
    return choice;
}

string Server::getUserInput()
{
    printColored("[?] Please enter your input: ", Color::YELLOW, false);
    string input;
    getline(cin, input);
    return input;
}

string Server::getFilePath()
{
    printColored("[?] Please enter the file path: ", Color::YELLOW, false);
    string filePath;
    getline(cin, filePath);
    return filePath;
}

string Server::saveToFile(const string &data)
{
    string fileName = "input/user_input.txt";
    ofstream file(fileName);
    if (!file)
    {
        printColored("[!] Failed to create file for user input.", Color::RED);
        return "";
    }
    file << data;
    file.close();
    return fileName;
}
