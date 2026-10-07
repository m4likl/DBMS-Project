#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

// Constructor: Initializes the buffer metadata
StaticBuffer::StaticBuffer() {
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        metainfo[bufferIndex].free = true;
        metainfo[bufferIndex].dirty = false;
        metainfo[bufferIndex].timeStamp = -1;
        metainfo[bufferIndex].blockNum = -1;
    }
}

// Destructor: Flushes all modified (dirty) blocks to the disk on shutdown
StaticBuffer::~StaticBuffer() {
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if (!metainfo[bufferIndex].free && metainfo[bufferIndex].dirty) {
            Disk::writeBlock(blocks[bufferIndex], metainfo[bufferIndex].blockNum);
        }
    }
}

int StaticBuffer::getBufferNum(int blockNum) {
    if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if (!metainfo[i].free && metainfo[i].blockNum == blockNum) {
            return i;
        }
    }
    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::getFreeBuffer(int blockNum) {
    // Check if blockNum is valid
    if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    // Increase the timeStamp in metaInfo of all occupied buffers
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if (!metainfo[i].free) {
            metainfo[i].timeStamp++;
        }
    }

    int bufferNum = -1;

    // Check if there is any free buffer available
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
        if (metainfo[i].free) {
            bufferNum = i;
            break;
        }
    }

    // If no free buffer is available, find the least recently used (LRU) buffer block
    if (bufferNum == -1) {
        int maxTimeStamp = -1;
        int lruBufferIndex = -1;

        for (int i = 0; i < BUFFER_CAPACITY; i++) {
            if (metainfo[i].timeStamp > maxTimeStamp) {
                maxTimeStamp = metainfo[i].timeStamp;
                lruBufferIndex = i;
            }
        }

        bufferNum = lruBufferIndex;

        // If the LRU buffer block is dirty, write its content back to disk before reuse
        if (metainfo[bufferNum].dirty) {
            Disk::writeBlock(blocks[bufferNum], metainfo[bufferNum].blockNum);
        }
    }

    // Update the metaInfo entry for the selected buffer slot
    metainfo[bufferNum].free = false;
    metainfo[bufferNum].dirty = false;
    metainfo[bufferNum].blockNum = blockNum;
    metainfo[bufferNum].timeStamp = 0;

    return bufferNum;
}

int StaticBuffer::setDirtyBit(int blockNum) {
    // Find the buffer index corresponding to the blockNum
    int bufferNum = getBufferNum(blockNum);

    // If block is not present in the buffer
    if (bufferNum == E_BLOCKNOTINBUFFER) {
        return E_BLOCKNOTINBUFFER;
    }

    // If blockNum is out of bounds
    if (bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND;
    }

    // Set the dirty bit to true
    metainfo[bufferNum].dirty = true;

    return SUCCESS;
}