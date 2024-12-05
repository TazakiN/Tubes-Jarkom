#include "segment_handler.hpp"
#include <cstring>
#include <stdexcept>
#include "utils.hpp"

const uint8_t DEFAULT_WINDOW_SIZE = 4;

// Constructor
SegmentHandler::SegmentHandler() : windowSize(DEFAULT_WINDOW_SIZE),
                                   currentSeqNum(0),
                                   currentAckNum(0),
                                   dataStream(nullptr),
                                   dataSize(0),
                                   dataIndex(0),
                                   segmentBuffer(nullptr)
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
        // for (uint8_t i = 0; i < windowSize; i++)
        // {
        //     if (segmentBuffer[i].payload != nullptr)
        //     {
        //         delete[] segmentBuffer[i].payload;
        //     }
        // }
        delete[] segmentBuffer;
        segmentBuffer = nullptr;
    }

    if (dataStream != nullptr)
    {
        delete[] dataStream;
        dataStream = nullptr;
    }
}

void SegmentHandler::initializeBuffer()
{
    if (segmentBuffer != nullptr)
    {
        for (uint8_t i = 0; i < windowSize; i++)
        {
            if (segmentBuffer[i].payloadSize = 0)
            {
                delete[] segmentBuffer[i].payload;
            }
        }
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
    uint8_t segmentsToGenerate = std::min(windowSize,
                                          static_cast<uint8_t>((remainingData + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE));

    for (uint8_t i = 0; i < segmentsToGenerate; i++)
    {
        uint32_t currentOffset = dataIndex + (i * MAX_PAYLOAD_SIZE);
        uint32_t currentSize = std::min(MAX_PAYLOAD_SIZE, dataSize - currentOffset);

        segmentBuffer[i] = createDataSegment(
            static_cast<uint8_t *>(dataStream) + currentOffset,
            currentSize);

        currentSeqNum += currentSize;
    }

    // Fill remaining buffer slots with empty segments if any
    for (uint8_t i = segmentsToGenerate; i < windowSize; i++)
    {
        segmentBuffer[i] = createSegment();
    }
    // printColored("[+] Generated " + std::to_string(segmentsToGenerate) + " segments", Color::GREEN);
}

void SegmentHandler::setDataStream(uint8_t *newDataStream, uint32_t newDataSize)
{
    cleanup();

    dataStream = new uint8_t[newDataSize];
    std::memcpy(dataStream, newDataStream, newDataSize);
    dataSize = newDataSize;
    dataIndex = 0;

    generateSegments();
}

void SegmentHandler::setCurrentNumbers(uint32_t seqNum, uint32_t ackNum)
{
    currentSeqNum = seqNum;
    currentAckNum = ackNum;
    generateSegments();
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
    if (dataIndex >= dataSize)
    {
        return nullptr;
    }

    Segment *currentWindow = new Segment[size];
    for (uint8_t i = 0; i < size; i++)
    {
        currentWindow[i] = segmentBuffer[i];

        memset(&segmentBuffer[i], 0, sizeof(Segment));
    }

    // Advance the data index
    dataIndex += size * MAX_PAYLOAD_SIZE;
    if (dataIndex > dataSize)
    {
        dataIndex = dataSize;
    }

    // Generate new segments for the window
    generateSegments();

    return currentWindow;
}