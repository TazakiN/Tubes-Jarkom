#include "client.hpp"
#include <iostream>
#include "utils.hpp"
#include <netinet/in.h>
#include <string.h>
#include <fstream>

Client::Client(const std::string &ip, int port)
{
    this->self_ip = ip;
    this->self_port = port;
    connection = new TCPSocket(ip, port);
    connection->listen();

    received_seg = 0;
}

void Client::run()
{
    printColored("[i] Node is now a receiver", Color::BLUE);
    getServerInfo();

    // Send SYN to the server
    sendSYN();

    // Receive the SYN-ACK from the server
    Segment synAckSeg;
    int32_t bytes_received = connection->recv(&synAckSeg, sizeof(synAckSeg));
    if (bytes_received > 0)
    {
        handleMessage(&synAckSeg, nullptr);
    }
    else
    {
        printColored("[!] [Handshake] Failed to receive SYN-ACK from server", Color::RED);
    }
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
    if (segment->flags == SYN_ACK_FLAG)
    {
        printColored("[+] [Handshake] [A=" + std::to_string(segment->ackNum) + "] [S=" + std::to_string(segment->seqNum) + "] Received SYN-ACK request from " + server_ip + ":" + std::to_string(server_port), Color::GREEN);
        sendACK(segment);
    }
    // TODO: Implement file receiving
    else if (segment->payloadSize > 0)
    {
        printColored("[+] Received data segment with sequence number: " + to_string(segment->seqNum), Color::GREEN);
        // print payload
        std::string payload((char *)segment->payload, segment->payloadSize);
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

    connection->send(server_ip, server_port, &ackSeg, sizeof(ackSeg));
    printColored("[+] [Handshake] [A=" + std::to_string(ackSeg.ackNum) + "] Sent ACK to " + server_ip + ":" + std::to_string(server_port), Color::GREEN);
    connection->setStatus(TCPStatusEnum::ESTABLISHED);
    printColored("[~] Connection established with " + server_ip + ":" + std::to_string(server_port), Color::PURPLE);

    initialSeqNum = segment->ackNum;
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

void Client::sendSYN()
{
    uint32_t client_seq_num = rand();

    Segment synSeg = syn(client_seq_num);

    // Send the SYN segment to the server
    printColored("[i] Trying to contact the sender at " + server_ip + ":" + std::to_string(server_port), Color::BLUE);
    connection->send(server_ip, server_port, &synSeg, sizeof(synSeg));
    printColored("[+] [Handshake] [S=" + std::to_string(synSeg.seqNum) + "] Sent SYN to server", Color::GREEN);

    connection->setStatus(TCPStatusEnum::SYN_SENT);
}

void Client::handleFileData(Segment *segment)
{
    if (!receivedData.empty() && segment->seqNum <= lastAckedSeqNum)
    {
        // Duplicate segment, ignore
        return;
    }

    uint32_t offset = segment->seqNum - initialSeqNum;

    if (receivedData.size() < offset + segment->payloadSize)
    {
        receivedData.resize(offset + segment->payloadSize);
    }

    memcpy(&receivedData[offset], segment->payload, segment->payloadSize);

    if (offset + segment->payloadSize > totalDataSize)
    {
        totalDataSize = offset + segment->payloadSize;
    }

    received_seg++;
    printColored("[+] [Established] [Seg " + to_string(received_seg) + "] [S=" + to_string(segment->seqNum) + "] ACKed", Color::GREEN);
    // Send ACK
    Segment ackSegment = ack(segment->seqNum + segment->payloadSize, segment->seqNum + 1);

    connection->send(server_ip, server_port, &ackSegment, sizeof(ackSegment));

    lastAckedSeqNum = segment->seqNum;
    printColored("[+] [Established] [Seg " + to_string(received_seg) + "] [A=" + to_string(ackSegment.ackNum) + "] Sent ACK", Color::GREEN);
}

void Client::handleFileTransferFin()
{
    printColored("[+] [Closing] Received FIN request from " + server_ip + ":" + std::to_string(server_port), Color::GREEN);

    string outputPath = "output/received_file.txt";
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile)
    {
        printColored("[!] Failed to create output file", Color::RED);
        return;
    }

    outFile.write(reinterpret_cast<const char *>(receivedData.data()), totalDataSize);
    outFile.close();

    printColored("[i] File received, saved " + std::to_string(totalDataSize) + " bytes to: " + outputPath, Color::BLUE);
    connection->setStatus(TCPStatusEnum::FIN_WAIT_1);
    // Send FIN-ACK
    Segment finAckSegment = finAck();
    connection->send(server_ip, server_port, &finAckSegment, sizeof(finAckSegment));
    printColored("[+] [Closing] Sending FIN-ACK request to " + server_ip + ":" + std::to_string(server_port), Color::GREEN);
}
