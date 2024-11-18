#include "segment.hpp"
#include <cstring>
#include <netinet/in.h>

Segment createSegment()
{
    Segment seg = {};
    seg.data_offset = 5;
    seg.reserved = 0;
    seg.window = 65535;
    seg.urgentPointer = 0;
    seg.payloadSize = 0;
    memset(seg.payload, 0, MAX_PAYLOAD_SIZE);
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

    size_t headerSize = sizeof(Segment);

    uint16_t buffer[headerSize / 2 + (headerSize % 2)];
    std::memcpy(buffer, &segment, headerSize);

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

Segment updateChecksum(Segment segment)
{
    uint8_t *checksumBytes = calculateChecksum(segment);
    segment.checksum = (checksumBytes[0] << 8) | checksumBytes[1];
    delete[] checksumBytes;
    return segment;
}

bool isValidChecksum(Segment segment)
{
    uint16_t originalChecksum = segment.checksum;
    uint8_t *calculatedChecksum = calculateChecksum(segment);
    uint16_t calculated = (calculatedChecksum[0] << 8) | calculatedChecksum[1];
    delete[] calculatedChecksum;

    return originalChecksum == calculated;
}