#include "server.hpp"
#include <iostream>
#include <fstream>
#include "utils.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

Server::Server(const string &ip, int port) : SWS(4), LAR(0), LFS(0)
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

        srand(getpid());
        handshakeSeqNum = rand();
        Segment synAckSegment = synAck(handshakeSeqNum);
        synAckSegment.ackNum = segment->seqNum + 1;
        initialSeqNum = segment->seqNum + 1;

        connection->sendTo(client_addr, &synAckSegment, sizeof(synAckSegment));

        printColored("[+] [Handshake] [A=" + to_string(synAckSegment.ackNum) + "] [S=" + to_string(synAckSegment.seqNum) + "] Sent SYN-ACK to " + client_ip + ":" + to_string(client_port), Color::GREEN);
        connection->setStatus(TCPStatusEnum::SYN_RECEIVED);
    }
    else if (segment->flags == ACK_FLAG)
    {
        if (connection->getStatus() == TCPStatusEnum::SYN_RECEIVED)
        {
            printColored("[+] [Handshake] [A=" + to_string(segment->ackNum) + "] Received ACK from " + client_ip + ":" + to_string(client_port), Color::GREEN);
            if (segment->ackNum == handshakeSeqNum + 1)
            {
                connection->setStatus(TCPStatusEnum::ESTABLISHED);
                printColored("[~] Connection established with " + client_ip + ":" + to_string(client_port), Color::PURPLE);

                SWS = DEFAULT_WINDOW_SIZE * MAX_PAYLOAD_SIZE;
                LAR = initialSeqNum - MAX_PAYLOAD_SIZE;
                LFS = LAR + SWS;

                initTransfer(segment, client_addr);
            }
            else
            {
                printColored("[!] [Handshake] ACK number doesn't match!", Color::RED);
            }
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
    printColored("[?] Please enter your input (max 1024 characters): ", Color::YELLOW, false);
    string input;
    getline(cin, input);
    if (input.length() > 1024)
    {
        printColored("[!] Input exceeds maximum allowed length.", Color::RED);
        return "";
    }
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
    segmentHandler->setCurrentNumbers(initialSeqNum, initialSeqNum);

    printColored("[+] Sending input to " + (string)inet_ntoa(client_addr->sin_addr) + ":" + to_string(ntohs(client_addr->sin_port)), Color::GREEN);

    sendNextWindow(client_addr);

    delete[] fileData;
}

void Server::sendNextWindow(struct sockaddr_in *client_addr)
{
    SegmentHandler *segmentHandler = connection->getSegmentHandler();
    uint8_t windowSize = segmentHandler->getWindowSize();
    int adv_size = (SWS - (LFS - LAR)) / MAX_PAYLOAD_SIZE;
    Segment *segments = segmentHandler->advanceWindow(adv_size);
    LFS = LAR + SWS;

    if (adv_size == 0)
    {
        adv_size = DEFAULT_WINDOW_SIZE;
    }

    if (segments == nullptr)
    {
        // All data has been sent
        Segment finSegment = fin();
        connection->sendTo(client_addr, &finSegment, sizeof(finSegment));
        printColored("[+] [Closing] Sending FIN request to " + (string)inet_ntoa(client_addr->sin_addr) + ":" + to_string(ntohs(client_addr->sin_port)), Color::GREEN);
        return;
    }

    for (int i = DEFAULT_WINDOW_SIZE - adv_size; i < DEFAULT_WINDOW_SIZE; i++)
    {
        if (segments[i].payloadSize > 0)
        {
            connection->sendTo(client_addr, &segments[i], sizeof(Segment));
            // if (rand()%2 == 1) {
            //     connection->sendTo(client_addr, &segments[i], sizeof(Segment));
            // }
            printColored("[+] [Established] [Seg " + to_string(((segments[i].seqNum-initialSeqNum) / MAX_PAYLOAD_SIZE) + 1) + "] [S=" + to_string(segments[i].seqNum) + "] Sent", Color::GREEN);

            startTimer(segments[i], client_addr);
        }
    }
    printColored("[~] [Established] Waiting for segments to be ACKed", Color::YELLOW);
}

void Server::startTimer(Segment segment, struct sockaddr_in *client_addr) {

    timers[segment.seqNum] = std::thread([this, segment, client_addr]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_INTERVAL));

        std::lock_guard<std::mutex> lock(timerMutex);

        if (timers.find(segment.seqNum) != timers.end()) {
            printColored("[!] [Timeout] [Seg " + std::to_string(((segment.seqNum-initialSeqNum) / MAX_PAYLOAD_SIZE) + 1) + "] [S=" + to_string(segment.seqNum) + "]. Retransmitting...", Color::RED);
            connection->sendTo(client_addr, (void*)&segment, sizeof(segment));
            // if (rand()%2 == 1) {
            //     connection->sendTo(client_addr, (void*)&segment, sizeof(segment));
            // }
            timers.erase(segment.seqNum);
            startTimer(segment, client_addr);
        }
    });

    timers[segment.seqNum].detach();
}


// sliding window
void Server::handleFileTransferAck(Segment *segment, struct sockaddr_in *client_addr) {
    if (segment->ackNum-MAX_PAYLOAD_SIZE > LAR) {
        printColored("[+] ACK received for packet with sequence number: " + std::to_string(LAR), Color::GREEN);
        {
            std::lock_guard<std::mutex> lock(timerMutex);
            for (int i = LAR+MAX_PAYLOAD_SIZE; i <= segment->ackNum-MAX_PAYLOAD_SIZE; i += MAX_PAYLOAD_SIZE){
                auto it = timers.find(i);
                if (it != timers.end()) {
                    timers.erase(it);
                }
            }
        }
        LAR = segment->ackNum - MAX_PAYLOAD_SIZE;
        sendNextWindow(client_addr);
    }
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