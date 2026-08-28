#include "StaticBuffer.h"

// The static member arrays must be explicitly defined here
unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {
  // Initialize all blocks as free
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty = false;
    metainfo[bufferIndex].timeStamp = -1;
    metainfo[bufferIndex].blockNum = -1;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {}

int StaticBuffer::getFreeBuffer(int blockNum) {
  // Check if blockNum is valid
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }
  
  int allocatedBuffer = -1;

  // iterate through all the blocks in the StaticBuffer
  // find the first free block in the buffer (check metainfo)
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (metainfo[i].free == true) {
      allocatedBuffer = i;
      break;
    }
  }

  // Check if no free buffer was found
  if (allocatedBuffer == -1) {
    return E_OUTOFBOUND; // Or a specific error like E_OUTOFMEMORY
  }

  // assign allocatedBuffer = index of the free block
  metainfo[allocatedBuffer].free = false;
  metainfo[allocatedBuffer].blockNum = blockNum;

  return allocatedBuffer;
}


/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS - 1)
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  // find and return the bufferIndex which corresponds to blockNum
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (metainfo[i].free == false && metainfo[i].blockNum == blockNum) {
      return i;
    }
  }

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}