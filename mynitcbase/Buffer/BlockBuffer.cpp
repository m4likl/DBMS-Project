#include "BlockBuffer.h"
#include <cstring>

// Constructor for BlockBuffer
BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

// Constructor for RecBuffer (calls base class BlockBuffer constructor)
RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}

/*
Used to get the header of the block into the location pointed to by `head`
NOTE: this function expects the caller to allocate memory for `head`
*/
int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occurred in the process
  }

  // Copy header fields directly from the buffer array
  memcpy(&head->blockType, bufferPtr, 4);
  memcpy(&head->pblock, bufferPtr + 4, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->numSlots, bufferPtr + 24, 4);

  return SUCCESS;
}

/*
Used to get the record at slot `slotNum` into the array `rec`
NOTE: this function expects the caller to allocate memory for `rec`
*/
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  // Calculate record start offset:
  // Offset = Header (32 bytes) + Slotmap (slotCount bytes) + preceding records
  int recordSize = attrCount * ATTR_SIZE;
  int offset = 32 + slotCount + (recordSize * slotNum);

  // Copy attributes into the array
  for (int i = 0; i < attrCount; i++) {
    memcpy(&rec[i], bufferPtr + offset + (i * ATTR_SIZE), ATTR_SIZE);
  }

  return SUCCESS;
}

/*
Used to load a block to the buffer and get a pointer to it.
NOTE: this function expects the caller to allocate memory for the argument
*/
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}