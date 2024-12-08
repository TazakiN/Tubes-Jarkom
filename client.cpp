#include "client.hpp"
#include <iostream>
#include "utils.hpp"
#include <netinet/in.h>
#include <string.h>
#include <fstream>
#include <arpa/inet.h>

Client::Client(const std::string &ip, int port)
{
    this->self_ip = ip;
    this->self_port = port;
    this->server_port = port; // Initialize server_port
    connection = new TCPSocket(ip, port);
    filenameReceived = false;
    CRC = 0;
    received_seg = 0;
}

void Client::run()
{
    printColored("[i] Node is now a receiver", Color::BLUE);
    getServerInfo();

    // Send SYN to the server
    sendSYN();

    // Receive the SYN-ACK from the server
    // Segment synAckSeg;
    // struct sockaddr_in src_addr;
    // int32_t bytes_received = connection->recvFrom(&synAckSeg, sizeof(synAckSeg), &src_addr);
    // server_addr = src_addr;

    // this->server_addr.sin_family = AF_INET;
    // this->server_addr.sin_port = htons(server_port);
    // this->server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

    // if (bytes_received > 0)
    // {
    //     handleMessage(&synAckSeg, nullptr);
    // }
    // else
    // {
    //     printColored("[!] [Handshake] Failed to receive SYN-ACK from server", Color::RED);
    // }
    while (true)
    {
        Segment receivedSegment;
        struct sockaddr_in src_addr;

        int32_t bytes_received = connection->recvFrom(&receivedSegment, sizeof(receivedSegment), &src_addr);
        if (bytes_received > 0)
        {
            handleMessage(&receivedSegment, &src_addr);
        }
        if (connection->getStatus() == TCPStatusEnum::FIN_WAIT_2)
        {
            printColored("[i] Node ended", Color::BLUE);
            break;
        }
    }
}

void Client::handleMessage(void *buffer, struct sockaddr_in *src_addr)
{
    Segment *segment = (Segment *)buffer;

    //Mengenalkan client mengenai tipe eror detection yang sedang dipakai
    if(segment->CRC == 1){
        CRC = 1;
    }

    // Apabila metode error detectionnya checksum (default)
    if ((segment->CRC == 0) && (!isValidChecksum(*segment)))
    {
        printColored("[!] Invalid checksum detected, discarding segment", Color::RED);
        return;
    }
    // Apabila metode error detectionnya CRC
    else if ((segment->CRC == 1) && (verifyCRC16(*segment)))
    {
        printColored("[!] Invalid CRC16 detected, discarding segment", Color::RED);
        return;
    }

    if (segment->flags == SYN_ACK_FLAG)
    {
        printColored("[+] [Handshake] [A=" + std::to_string(segment->ackNum) + "] [S=" + std::to_string(segment->seqNum) + "] Received SYN-ACK request from " + server_ip + ":" + std::to_string(server_port), Color::GREEN);
        if (segment->ackNum == initialSeqNum + 1)
        {
            RWS = DEFAULT_WINDOW_SIZE * MAX_PAYLOAD_SIZE;
            LFR = segment->ackNum - MAX_PAYLOAD_SIZE;
            LAF = LFR + RWS;
            sendACK(segment);
        }
        else
        {
            printColored("[!] [Handshake] ACK number doesn't match!", Color::RED);
        }
    }
    // TODO: Implement file receiving
    else if (segment->payloadSize > 0)
    {
        printColored("[+] Received data segment with sequence number: " + to_string(segment->seqNum), Color::GREEN);
        // print payload
        // std::string payload((char *)segment->payload, segment->payloadSize);

        handleFileData(segment);
    }
    else if (segment->flags == FIN_FLAG)
    {
        handleFileTransferFin();
    }
    else if (segment->flags == ACK_FLAG)
    {
        if (connection->getStatus() == TCPStatusEnum::FIN_WAIT_1)
        {
            printColored("[+] [Closing]  Received ACK request from " + server_ip + ":" + std::to_string(server_port), Color::GREEN);
            printColored("[i] Connection closed successfully", Color::BLUE);
            connection->setStatus(TCPStatusEnum::FIN_WAIT_2);
        }
    }
    else
    {
        printColored("[!] Received unexpected segment", Color::RED);
    }
}

void Client::sendACK(Segment *segment)
{
    Segment ackSeg = ack(segment->ackNum, segment->seqNum + 1);

    connection->sendTo(&server_addr, &ackSeg, sizeof(ackSeg));
    printColored("[+] [Handshake] [A=" + std::to_string(ackSeg.ackNum) + "] Sent ACK to " + server_ip + ":" + std::to_string(server_port), Color::GREEN);
    connection->setStatus(TCPStatusEnum::ESTABLISHED);
    printColored("[~] Connection established with " + server_ip + ":" + std::to_string(server_port), Color::PURPLE);

    initialSeqNum = segment->ackNum;
}

void Client::getServerInfo()
{
    printColored("[?] Please enter the server ip: ", Color::YELLOW, false);
    string ip;
    std::cin >> ip;
    this->server_ip = ip;

    printColored("[?] Please enter the server port: ", Color::YELLOW, false);
    int port;
    std::cin >> port;
    server_port = (int)port;
}

void Client::sendSYN()
{
    srand(getpid());
    initialSeqNum = rand();
    Segment synSeg = syn(initialSeqNum);
    synSeg = updateChecksum(synSeg);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);
    servaddr.sin_addr.s_addr = inet_addr(server_ip.c_str());

    int retries = 1000;
    bool synAckReceived = false;

    for (int i = 0; i < retries && !synAckReceived; ++i)
    {
        printColored("[i] Sending SYN attempt " + std::to_string(i + 1) + " to server at port " + std::to_string(server_port), Color::BLUE);
        printColored("[+] [Handshake] [S=" + std::to_string(synSeg.seqNum) + "] Sent SYN to server", Color::GREEN);
        connection->sendTo(&servaddr, &synSeg, sizeof(synSeg));
        connection->setStatus(TCPStatusEnum::SYN_SENT);

        struct sockaddr_in src_addr;
        Segment synAckSeg;
        connection->setTimeout(5000);
        int32_t bytes_received = connection->recvFrom(&synAckSeg, sizeof(synAckSeg), &src_addr);
        server_addr = src_addr;
        server_ip = inet_ntoa(servaddr.sin_addr);
        server_port = ntohs(server_addr.sin_port);

        if (bytes_received > 0 && synAckSeg.flags == SYN_ACK_FLAG)
        {
            synAckReceived = true;
            connection->unsetTimeout();
            handleMessage(&synAckSeg, &src_addr);
        }
        else
        {
            printColored("[!] Timeout waiting for SYN-ACK, retrying...", Color::RED);
        }
    }

    if (!synAckReceived)
    {
        printColored("[!] Failed to establish connection after " + std::to_string(retries) + " attempts", Color::RED);
    }
}

void Client::handleFileData(Segment *segment)
{
    // Apabila metode error detectionnya checksum (default)
    if ((segment->CRC == 0) && (!isValidChecksum(*segment)))
    {
        printColored("[!] Invalid checksum in data segment, discarding", Color::RED);
        return;
    }
    // Apabila metode error detectionnya CRC
    else if ((segment->CRC == 1) && (verifyCRC16(*segment)))
    {
        printColored("[!] Invalid CRC16 in data segment, discarding", Color::RED);
        return;
    }

    if (segment->seqNum <= LFR || segment->seqNum > LAF)
    {
        if (segment->seqNum <= LFR) {
            Segment ackSeg = ack(LFR, LFR + MAX_PAYLOAD_SIZE);
            // if (rand()%2 == 1) {
            //     connection->sendTo(&server_addr, &ackSeg, sizeof(ackSeg));
            // }
            connection->sendTo(&server_addr, &ackSeg, sizeof(ackSeg));
        } else {
            printColored("[!] Out-of-window packet discarded: SeqNum=" + to_string(segment->seqNum), Color::RED);
        }
    }

    if (!receivedData.empty() && segment->seqNum <= lastAckedSeqNum)
    {
        printColored("[!] Duplicate: SeqNum=" + to_string(segment->seqNum), Color::RED);
        return;
    }

    uint32_t offset = segment->seqNum - initialSeqNum;

    if (offset + segment->payloadSize > totalDataSize)
    {
        totalDataSize = offset + segment->payloadSize;
    }

    // Validate filename in every segment
    if (!filenameReceived && strlen(segment->filename) > 0)
    {
        filename = std::string(segment->filename);
        filenameReceived = true;
        printColored("[+] [Established] [Seg " + to_string(((segment->seqNum - initialSeqNum) / MAX_PAYLOAD_SIZE) + 1) +
                         "] [S=" + to_string(segment->seqNum) + "] Filename received: " + filename,
                     Color::GREEN);
    }
    else if (filenameReceived && strcmp(segment->filename, filename.c_str()) != 0)
    {
        printColored("[!] Filename mismatch in segment", Color::RED);
        return;
    }

    receivedData.resize(std::max(receivedData.size(), static_cast<size_t>(totalDataSize)));
    receivedData.insert(receivedData.begin() + offset,
                        segment->payload,
                        segment->payload + segment->payloadSize);
    receivedIndex.resize(std::max(receivedIndex.size(), static_cast<size_t>(offset / MAX_PAYLOAD_SIZE) + 1));
    receivedIndex.insert(receivedIndex.begin() + (offset / MAX_PAYLOAD_SIZE), 1);

    if (isContiguous(LFR + MAX_PAYLOAD_SIZE, segment->seqNum))
    {
        LFR = segment->seqNum;
        LAF = LFR + RWS;
        lastAckedSeqNum = segment->seqNum;
        sendFileACK(segment);
        printColored("[~] [Established] Waiting for segments to be sent", Color::YELLOW);
    }
}

void Client::sendFileACK(Segment *segment)
{
    Segment ackSeg = ack(segment->seqNum, segment->seqNum + MAX_PAYLOAD_SIZE);
    connection->sendTo(&server_addr, &ackSeg, sizeof(ackSeg));
    // printColored("[+] Sent ACK: SeqNum=" + to_string(ackSeg.ackNum), Color::GREEN);
    printColored("[+] [Established] [Seg " + to_string(((segment->seqNum - initialSeqNum) / MAX_PAYLOAD_SIZE) + 1) + "] [A=" + to_string(ackSeg.ackNum) + "] Sent ACK", Color::GREEN);
}

void Client::handleFileTransferFin()
{
    printColored("[+] [Closing] Received FIN request from " + server_ip + ":" + std::to_string(server_port), Color::GREEN);

    string outputPath = "output/" + string(filename);
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile)
    {
        printColored("[!] Failed to create output file", Color::RED);
        return;
    }


    std::cout << std::endl;
    outFile.write(reinterpret_cast<const char *>(receivedData.data()), receivedData.size() - 4);
    outFile.close();
    
    printColored("[i] File received, saved " + std::to_string(totalDataSize) + " bytes to: " + outputPath, Color::BLUE);
    connection->setStatus(TCPStatusEnum::FIN_WAIT_1);

    // Send FIN-ACK
    Segment finAckSegment = finAck();
    connection->sendTo(&server_addr, &finAckSegment, sizeof(finAckSegment));
    printColored("[+] [Closing] Sending FIN-ACK request to " + server_ip + ":" + std::to_string(server_port), Color::GREEN);
}

bool Client::isContiguous(uint32_t start, uint32_t end)
{
    for (uint32_t seq = start; seq <= end; seq += MAX_PAYLOAD_SIZE)
    {
        uint32_t offset = seq - initialSeqNum;
        if (!receivedIndex[offset / MAX_PAYLOAD_SIZE])
        {
            return false;
        }
    }
    return true;
}