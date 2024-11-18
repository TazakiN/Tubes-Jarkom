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

    bool retFlag;
    handleInputMethod(mode, retFlag);

    if (retFlag)
        return;

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

void Server::handleInputMethod(int mode, bool &retFlag)
{
    retFlag = true;
    if (mode == 1)
    {
        string userInput = getUserInput(); // Input manual disimpan ke file
        if (!userInput.empty())
        {
            filePath = saveToFile(userInput);
            if (!filePath.empty())
            {
                printColored("[!] User input saved to " + filePath, Color::BLUE);
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
    retFlag = false;
}

void Server::handleMessage(void *buffer, struct sockaddr_in *client_addr)
{
    Segment *segment = (Segment *)buffer;

    string client_ip = inet_ntoa(client_addr->sin_addr);
    int client_port = ntohs(client_addr->sin_port);

    if (segment->flags == SYN_FLAG)
    {
        printColored("[+] [Handshake] [S=" + to_string(segment->seqNum) + "] Received SYN from " + client_ip + ":" + to_string(client_port), Color::GREEN);

        Segment synAckSegment = synAck(segment->seqNum);
        synAckSegment.ackNum = segment->seqNum + 1;

        connection->sendTo(client_addr, &synAckSegment, sizeof(synAckSegment));

        printColored("[+] [Handshake] [A=" + to_string(synAckSegment.ackNum) + "] [S=" + to_string(synAckSegment.seqNum) + "] Sent SYN-ACK to " + client_ip + ":" + to_string(client_port), Color::GREEN);

        uint32_t server_seq_num = synAckSegment.ackNum;
        connection->setStatus(TCPStatusEnum::SYN_RECEIVED);
    }
    else if (segment->flags == ACK_FLAG)
    {
        // TODO: Implement file sending
        if (connection->getStatus() == TCPStatusEnum::SYN_RECEIVED)
        {
            printColored("[+] [Handshake] [A=" + to_string(segment->ackNum) + "] Received ACK from " + client_ip + ":" + to_string(client_port), Color::GREEN);
            connection->setStatus(TCPStatusEnum::ESTABLISHED);
            printColored("[~] Connection established with " + client_ip + ":" + to_string(client_port), Color::PURPLE);

            // TODO: Implement file sending
            initTransfer(segment, client_addr);
        }
        else if (connection->getStatus() == TCPStatusEnum::ESTABLISHED)
        { // Handle ACK for file transfer
            handleFileTransferAck(segment, client_addr);
        }
    }
    else if (segment->flags == FIN_ACK_FLAG)
    {
        handleFINACK(segment, client_addr);
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

void Server::initTransfer(Segment *segment, struct sockaddr_in *client_addr)
{
    // Read file content
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        printColored("[!] Failed to open file for transfer", Color::RED);
        return;
    }

    file.seekg(0, std::ios::end);
    uint32_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    uint8_t *fileData = new uint8_t[fileSize];
    file.read(reinterpret_cast<char *>(fileData), fileSize);
    file.close();

    SegmentHandler *segmentHandler = connection->getSegmentHandler();
    segmentHandler->setDataStream(fileData, fileSize);
    segmentHandler->setCurrentNumbers(segment->ackNum, segment->seqNum + 1);

    printColored("[+] Sending input to " + (string)inet_ntoa(client_addr->sin_addr) + ":" + to_string(ntohs(client_addr->sin_port)), Color::GREEN);

    sendNextWindow(client_addr);

    delete[] fileData;
}

void Server::sendNextWindow(struct sockaddr_in *client_addr)
{
    SegmentHandler *segmentHandler = connection->getSegmentHandler();
    uint8_t windowSize = segmentHandler->getWindowSize();
    Segment *segments = segmentHandler->advanceWindow(windowSize);

    if (segments == nullptr)
    {
        // All data has been sent
        Segment finSegment = fin();
        connection->sendTo(client_addr, &finSegment, sizeof(finSegment));
        printColored("[+] [Closing] Sending FIN request to " + (string)inet_ntoa(client_addr->sin_addr) + ":" + to_string(ntohs(client_addr->sin_port)), Color::GREEN);
        return;
    }

    // Send all segments in the window
    for (uint8_t i = 0; i < windowSize; i++)
    {
        if (segments[i].payloadSize > 0)
        {
            connection->sendTo(client_addr, &segments[i], sizeof(Segment));
            printColored("[+] [Established] [Seg " + to_string(i + 1) + "] [S=" + to_string(segments[i].seqNum) + "] Sent", Color::GREEN);
        }
    }

    delete[] segments;
}

void Server::handleFileTransferAck(Segment *segment, struct sockaddr_in *client_addr)
{
    int ackIter = 1;
    printColored("[+] [Established] [Seg " + to_string(ackIter) + "] [A=" + to_string(segment->ackNum) + "] ACKed", Color::GREEN);

    uint8_t ackedSegments = 1;
    sendNextWindow(client_addr);
}

void Server::handleFINACK(Segment *segment, struct sockaddr_in *client_addr)
{
    printColored("[+] [Closing] Received FIN-ACK from " + (string)inet_ntoa(client_addr->sin_addr) + ":" + to_string(ntohs(client_addr->sin_port)), Color::GREEN);
    Segment ackSeg = ack(segment->ackNum, segment->seqNum + 1);
    connection->sendTo(client_addr, &ackSeg, sizeof(ackSeg));
    printColored("[+] [Closing] Sending ACK request to " + (string)inet_ntoa(client_addr->sin_addr) + ":" + to_string(ntohs(client_addr->sin_port)), Color::GREEN);
    printColored("[i] Connection closed successfully", Color::BLUE);
    connection->setStatus(TCPStatusEnum::CLOSED);
}