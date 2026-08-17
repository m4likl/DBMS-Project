#include "StaticBuffer.h"


// The static member arrays must be explicitly defined here
unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {
  // initialise all blocks as free
 
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stag for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
    metainfo[bufferIndex].free = true;
  }es, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {}

int StaticBuffer::getFreeBuffer(int blockNum) {
  // Check if blockNum is valid
  // Note: DISK_BLOCKS is the total number of blocks, so valid indices are 0 to DISK_BLOCKS - 1
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
  // and return E_OUTOFBOUND if not valid.
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (metainfo[i].free == false && metainfo[i].blockNum == blockNum) {
      return i;
    }
  }

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}