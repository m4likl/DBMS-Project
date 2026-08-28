

#include "BlockBuffer.h"
#include <cstring>

// BlockBuffer Constructor
BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

// RecBuffer Constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}

// Loads block into static buffer and gets buffer pointer
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char** bufferPtr) {
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  // ADD THIS CHECK: If blockNum is invalid, return immediately
  if (bufferNum == E_OUTOFBOUND) {
    return E_OUTOFBOUND;
  }

  
  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }
    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }
  
  *bufferPtr = StaticBuffer::blocks[bufferNum];
  return SUCCESS;
}

// Gets block header info
int BlockBuffer::getHeader(HeadInfo* head) {
  unsigned char* bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) return ret;

  memcpy(head, bufferPtr, sizeof(HeadInfo));
  return SUCCESS;
}

// Reads record from specific slot
int RecBuffer::getRecord(Attribute* record, int slotNum) {
  unsigned char* bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) return ret;

  HeadInfo head;
  getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  if (slotNum < 0 || slotNum >= slotCount) {
    return E_OUTOFBOUND;
  }

  unsigned char* slotPointer = bufferPtr + HEADER_SIZE + slotCount + (slotNum * attrCount * sizeof(Attribute));
  memcpy(record, slotPointer, attrCount * sizeof(Attribute));
  return SUCCESS;
}
  
/* used to get the slotmap from a record block */
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block
  ret = getHeader(&head);
  if (ret != SUCCESS) {
    return ret;
  }

  int slotCount = head.numSlots;

  // offset by HEADER_SIZE to locate the slotmap in memory
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy slotmap buffer
  memcpy(slotMap, slotMapInBuffer, slotCount);

  return SUCCESS;
}

/* Compares two Attribute values based on attrType (NUMBER or STRING) */
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {
  double diff;

  if (attrType == NUMBER) {
    diff = attr1.nVal - attr2.nVal;
  } else { // attrType == STRING
    diff = strcmp(attr1.sVal, attr2.sVal);
  }

  if (diff < 0) return -1;
  if (diff > 0) return 1;
  return 0;
}