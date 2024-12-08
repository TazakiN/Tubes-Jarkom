#include "segment_handler.hpp"
#include <cstring>
#include <stdexcept>
#include "utils.hpp"
#include <string.h>
#include <string>

// Constructor
SegmentHandler::SegmentHandler() : windowSize(DEFAULT_WINDOW_SIZE),
                                   currentSeqNum(0),
                                   currentAckNum(0),
                                   dataStream(nullptr),
                                   dataSize(0),
                                   dataIndex(0),
                                   segmentBuffer(nullptr),
                                   filename()
{
}

SegmentHandler::~SegmentHandler()
{
    cleanup();
}

void SegmentHandler::cleanup()
{
    if (segmentBuffer != nullptr)
    {
        delete[] segmentBuffer;
        segmentBuffer = nullptr;
    }

    if (dataStream != nullptr)
    {
        delete[] static_cast<uint8_t *>(dataStream);
        dataStream = nullptr;
    }
}

void SegmentHandler::setFileName(const char *newFilename)
{
    strncpy(filename, newFilename, MAX_FILENAME_SIZE - 1);
    filename[MAX_FILENAME_SIZE - 1] = '\0';
}

void SegmentHandler::initializeBuffer()
{
    if (segmentBuffer != nullptr)
    {
        delete[] segmentBuffer;
        segmentBuffer = nullptr;
    }
    segmentBuffer = new Segment[windowSize];
}

Segment SegmentHandler::createDataSegment(const uint8_t *data, uint32_t size)
{
    Segment segment = createSegment();

    segment.seqNum = currentSeqNum;
    segment.ackNum = currentAckNum;
    segment.flags = 0;

    // Set payload
    memset(segment.payload, 0, MAX_PAYLOAD_SIZE);
    std::memcpy(segment.payload, data, size);
    segment.payloadSize = size;

    return updateChecksum(segment);
}

void SegmentHandler::generateSegments()
{
    if (dataStream == nullptr || dataSize == 0)
    {
        throw std::runtime_error("No data stream set");
    }

    initializeBuffer();

    uint32_t remainingData = dataSize - dataIndex;
    uint8_t segmentsToGenerate = std::min(static_cast<uint32_t>(windowSize),
                                          (remainingData + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE);

    for (uint8_t i = 0; i < segmentsToGenerate; i++)
    {
        uint32_t currentOffset = dataIndex + (i * MAX_PAYLOAD_SIZE);
        uint32_t currentSize = std::min(MAX_PAYLOAD_SIZE, dataSize - currentOffset);

        segmentBuffer[i] = createDataSegment(
            static_cast<uint8_t *>(dataStream) + currentOffset,
            currentSize);
        strncpy(segmentBuffer[i].filename, filename, MAX_FILENAME_SIZE - 1);
        segmentBuffer[i].filename[MAX_FILENAME_SIZE - 1] = '\0';
        currentSeqNum += currentSize;
    }

    // Fill remaining buffer slots with empty segments if any
    for (uint8_t i = segmentsToGenerate; i < windowSize; i++)
    {
        segmentBuffer[i] = createSegment();
    }
}

void SegmentHandler::setDataStream(uint8_t *newDataStream, uint32_t newDataSize)
{
    cleanup();

    dataStream = new uint8_t[newDataSize];
    std::memcpy(dataStream, newDataStream, newDataSize);
    dataSize = newDataSize;
    dataIndex = 0;
}

void SegmentHandler::setCurrentNumbers(uint32_t seqNum, uint32_t ackNum)
{
    currentSeqNum = seqNum;
    currentAckNum = ackNum;
}

uint8_t SegmentHandler::getWindowSize()
{
    return windowSize;
}

Segment *SegmentHandler::advanceWindow(uint8_t size)
{
    if (size > windowSize)
    {
        throw std::invalid_argument("Advance size cannot be larger than window size");
    }

    // kalo udah semua data kekirim, return nullptr
    dataIndex += size * MAX_PAYLOAD_SIZE;
    if (dataIndex > dataSize)
    {
        dataIndex = dataSize;
    }

    if (dataIndex >= dataSize)
    {
        return nullptr;
    }

    setCurrentNumbers(currentAckNum + size * MAX_PAYLOAD_SIZE, currentAckNum + size * MAX_PAYLOAD_SIZE);

    // Generate new segments for the window
    generateSegments();

    Segment *currentWindow = new Segment[DEFAULT_WINDOW_SIZE];
    for (uint8_t i = 0; i < DEFAULT_WINDOW_SIZE; i++)
    {
        currentWindow[i] = segmentBuffer[i];
        memset(&segmentBuffer[i], 0, sizeof(Segment));
    }

    return currentWindow;
}