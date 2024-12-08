#include "segment.hpp"
#include <cstring>
#include "utils.hpp"
#include <netinet/in.h>
#include <iostream>
#include <ostream>
#define CRC16_POLY 0x8005   // CRC-16: 1 + x² + x¹⁵ + x¹⁶
#define CRC16_INITIAL 0xFFFF // Inisialisasi nilai CRC

Segment createSegment()
{
    Segment seg = {};
    seg.data_offset = 5;
    seg.reserved = 0;
    seg.window = 65535;
    seg.urgentPointer = 0;
    seg.payloadSize = 0;
    memset(seg.payload, 0, MAX_PAYLOAD_SIZE);

    //Default
    seg.CRC = 0;
    return seg;
}

Segment syn(uint32_t seqNum)
{
    Segment seg = createSegment();
    seg.seqNum = seqNum;
    seg.flags = SYN_FLAG;
    return updateChecksum(seg);
}

Segment ack(uint32_t seqNum, uint32_t ackNum)
{
    Segment seg = createSegment();
    seg.seqNum = seqNum;
    seg.ackNum = ackNum;
    seg.flags = ACK_FLAG;
    return updateChecksum(seg);
}

Segment synAck(uint32_t seqNum)
{
    Segment seg = createSegment();
    seg.seqNum = seqNum;
    seg.flags = SYN_ACK_FLAG;
    return updateChecksum(seg);
}

Segment fin()
{
    Segment seg = createSegment();
    seg.flags = FIN_FLAG;
    return updateChecksum(seg);
}

Segment finAck()
{
    Segment seg = createSegment();
    seg.flags = FIN_ACK_FLAG;
    return updateChecksum(seg);
}

uint8_t *calculateChecksum(Segment segment)
{
    segment.checksum = 0;

    size_t headerSize = segment.payloadSize;

    uint16_t buffer[headerSize / 2 + (headerSize % 2)];
    std::memcpy(buffer, &segment.payload, headerSize);

    size_t length = headerSize / 2;
    uint32_t sum = 0;
    for (size_t i = 0; i < length; i++)
    {
        sum += ntohs(buffer[i]);
        if (sum & 0x80000000)
        {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }
    if (headerSize & 1)
    {
        sum += ((uint8_t *)buffer)[headerSize - 1];
    }
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t checksum = ~sum;

    uint8_t *result = new uint8_t[2];
    result[0] = (checksum >> 8) & 0xFF;
    result[1] = checksum & 0xFF;

    return result;
}

Segment updateChecksum(Segment segment) {
    uint8_t *checksumBytes = calculateChecksum(segment);
    segment.checksum = (checksumBytes[0] << 8) | checksumBytes[1];
    delete[] checksumBytes;
    return segment;
}

bool isValidChecksum(Segment segment) {
    uint16_t originalChecksum = segment.checksum;
    uint8_t *calculatedChecksum = calculateChecksum(segment);
    uint16_t calculated = (calculatedChecksum[0] << 8) | calculatedChecksum[1];
    delete[] calculatedChecksum;
    return originalChecksum == calculated;
}
uint16_t calculateCRC16(Segment segment) {
    uint16_t crc = CRC16_INITIAL;
    size_t segmentSize = sizeof(Segment) - MAX_PAYLOAD_SIZE + segment.payloadSize;
    uint8_t *buffer = new uint8_t[segmentSize];
    std::memcpy(buffer, &segment, segmentSize);

    for (size_t i = 0; i < segmentSize; ++i) {
        crc ^= (buffer[i] << 8);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC16_POLY;
            } else {
                crc <<= 1;
            }
        }
    }

    delete[] buffer;
    return crc;
}

Segment appendCRC16(Segment segment) {
    uint16_t crc = calculateCRC16(segment);
    segment.payload[segment.payloadSize] = (crc >> 8) & 0xFF;
    segment.payload[segment.payloadSize + 1] = crc & 0xFF;
    segment.payloadSize += 2;
    return segment;
}

bool verifyCRC16(Segment segment) {
    uint16_t crc = calculateCRC16(segment);
    return crc == 0;
}
