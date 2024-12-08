#ifndef SEGMENT_HANDLER_HPP
#define SEGMENT_HANDLER_HPP

#include "segment.hpp"

const int MAX_FILENAME_SIZE = 255;

class SegmentHandler
{
private:
    uint8_t windowSize;
    uint32_t currentSeqNum;
    uint32_t currentAckNum;
    void *dataStream;
    uint32_t dataSize;
    uint32_t dataIndex;
    Segment *segmentBuffer; // or use std vector if you like
    char filename[MAX_FILENAME_SIZE];

    void cleanup();

    void initializeBuffer();

    Segment createDataSegment(const uint8_t *data, uint32_t size, unsigned int CRC);

    void generateSegments(unsigned int CRC);

public:
    SegmentHandler();
    ~SegmentHandler();
    void setDataStream(uint8_t *dataStream, uint32_t dataSize);
    void setFileName(const char *newFilename);
    void setCurrentNumbers(uint32_t seqNum, uint32_t ackNum);
    uint8_t getWindowSize();
    Segment *advanceWindow(uint8_t size, unsigned int CRC);
};

#endif