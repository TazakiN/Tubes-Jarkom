#ifndef SEGMENT_HPP
#define SEGMENT_HPP

#include <cstdint>
#include <cstddef>

const uint32_t MAX_PAYLOAD_SIZE = 1460;
// const uint32_t MAX_PAYLOAD_SIZE = 100;
const uint8_t DEFAULT_WINDOW_SIZE = 4;
struct Segment
{
    uint16_t sourcePort : 16;
    uint16_t destPort : 16;
    uint32_t seqNum : 32;
    uint32_t ackNum : 32;

    struct
    {
        unsigned int data_offset : 4;
        unsigned int reserved : 4;
    }; // 8 bits

    // struct
    // {
    //     unsigned int cwr : 1;
    //     unsigned int ece : 1;
    //     unsigned int urg : 1;
    //     unsigned int ack : 1;
    //     unsigned int psh : 1;
    //     unsigned int rst : 1;
    //     unsigned int syn : 1;
    //     unsigned int fin : 1; // tapi disini cuman bakal pake ack, syn, fin doang
    // } flags;                  // total ada 8 bit

    uint8_t flags : 8;

    uint16_t window : 16;
    uint16_t checksum : 16;
    uint16_t urgentPointer : 16;

    // options
    char filename[255];

    uint16_t payloadSize : 16;
    uint8_t payload[MAX_PAYLOAD_SIZE];
} __attribute__((packed));

const uint8_t FIN_FLAG = 1;
const uint8_t SYN_FLAG = 2;
const uint8_t ACK_FLAG = 16;
const uint8_t SYN_ACK_FLAG = SYN_FLAG | ACK_FLAG;
const uint8_t FIN_ACK_FLAG = FIN_FLAG | ACK_FLAG;

/**
 * Create a new Segment with default values
 */
Segment createSegment();

/**
 * Generate Segment that contain SYN packet
 */
Segment syn(uint32_t seqNum);

/**
 * Generate Segment that contain ACK packet
 */
Segment ack(uint32_t seqNum, uint32_t ackNum);

/**
 * Generate Segment that contain SYN-ACK packet
 */
Segment synAck(uint32_t seqNum);

/**
 * Generate Segment that contain FIN packet
 */
Segment fin();

/**
 * Generate Segment that contain FIN-ACK packet
 */
Segment finAck();

// update return type as needed
uint8_t *calculateChecksum(Segment segment);

/**
 * Return a new segment with a calculated checksum fields
 */
Segment updateChecksum(Segment segment);

/**
 * Check if a TCP Segment has a valid checksum
 */
bool isValidChecksum(Segment segment);


uint16_t calculateCRC16(const uint8_t *data, size_t length);

void appendCRC16(uint8_t *data, size_t length);

bool verifyCRC16(const uint8_t *data, size_t length);

#endif