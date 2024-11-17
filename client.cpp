#include "client.hpp"
#include <iostream>
#include "utils.hpp"
#include <netinet/in.h>

Client::Client(const std::string &ip, int port)
{
    this->self_ip = ip;
    this->self_port = port;
    connection = new TCPSocket(ip, port);
    connection->listen();
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
    // Initialize sequence numbers
    uint32_t client_seq_num = rand();

    // Create a SYN segment
    printColored("[~] Generating SYN segment", Color::PURPLE);
    Segment synSeg = syn(client_seq_num);

    // Send the SYN segment to the server
    printColored("[i] Trying to contact the sender at " + server_ip + ":" + std::to_string(server_port), Color::BLUE);
    connection->send(server_ip, server_port, &synSeg, sizeof(synSeg));
    printColored("[+] [Handshake] [S=" + std::to_string(synSeg.seqNum) + "] Sent SYN to server", Color::GREEN);

    connection->setStatus(TCPStatusEnum::SYN_SENT);
}
