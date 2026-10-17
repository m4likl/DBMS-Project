#include "BlockBuffer.h"
#include <cstring>

// BlockBuffer Constructor 2 (Access existing block)
BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

// BlockBuffer Constructor 1 (Allocate new block)
BlockBuffer::BlockBuffer(char blockType) {
  int blockTypeInt = (blockType == 'R') ? REC : UNUSED_BLK;
  int allocatedBlockNum = getFreeBlock(blockTypeInt);
  this->blockNum = allocatedBlockNum;
}

// RecBuffer Constructor 1 (Allocate new record block)
RecBuffer::RecBuffer() : BlockBuffer('R') {}

// RecBuffer Constructor 2 (Access existing record block)
RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}

// Gets block number of the block
int BlockBuffer::getBlockNum() {
    return this->blockNum;
}

// Checks if block is in buffer; loads from disk if not
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum != E_BLOCKNOTINBUFFER) {
    for (int i = 0; i < BUFFER_CAPACITY; i++) {
      if (!StaticBuffer::metainfo[i].free) {
        StaticBuffer::metainfo[i].timeStamp++;
      }
    }
    StaticBuffer::metainfo[bufferNum].timeStamp = 0;
  } else {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
    if (bufferNum < 0) {
      return bufferNum;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  *buffPtr = StaticBuffer::blocks[bufferNum];
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

// Sets block header info
int BlockBuffer::setHeader(struct HeadInfo *head) {
  unsigned char *bufferPtr;
  int status = loadBlockAndGetBufferPtr(&bufferPtr);
  if (status != SUCCESS) return status;

  // Copy the entire HeadInfo struct into the buffer header
  memcpy(bufferPtr, head, sizeof(HeadInfo));

  status = StaticBuffer::setDirtyBit(this->blockNum);
  if (status != SUCCESS) return status;

  return SUCCESS;
}

// Sets block type in buffer and block allocation map
int BlockBuffer::setBlockType(int blockType) {
  unsigned char *bufferPtr;
  int status = loadBlockAndGetBufferPtr(&bufferPtr);
  if (status != SUCCESS) return status;

  *((int32_t *)bufferPtr) = blockType;
  StaticBuffer::blockAllocMap[this->blockNum] = (unsigned char)blockType;

  status = StaticBuffer::setDirtyBit(this->blockNum);
  if (status != SUCCESS) return status;

  return SUCCESS;
}

// Allocates a free disk block and initializes its header
int BlockBuffer::getFreeBlock(int blockType) {
  int freeBlockNum = -1;

  for (int i = 0; i < DISK_BLOCKS; i++) {
    if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK) {
      freeBlockNum = i;
      break;
    }
  }

  if (freeBlockNum == -1) {
    return E_DISKFULL;
  }

  this->blockNum = freeBlockNum;

  int bufferIndex = StaticBuffer::getFreeBuffer(freeBlockNum);
  if (bufferIndex < 0) {
    return bufferIndex;
  }

  struct HeadInfo head;
  head.blockType = blockType;
  head.pblock = -1;
  head.lblock = -1;
  head.rblock = -1;
  head.numEntries = 0;
  head.numAttrs = 0;
  head.numSlots = 0;

  int status = setHeader(&head);
  if (status != SUCCESS) return status;

  status = setBlockType(blockType);
  if (status != SUCCESS) return status;

  return freeBlockNum;
}
// Reads record from specific slot
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) return ret;

    struct HeadInfo head;
    this->getHeader(&head);

    int numAttrs = head.numAttrs;
    int numSlots = head.numSlots;

    if (slotNum < 0 || slotNum >= numSlots) {
        return E_OUTOFBOUND;
    }

    unsigned char *slotMap = bufferPtr + HEADER_SIZE;
    if (slotMap[slotNum] == SLOT_UNOCCUPIED) {
        return E_FREESLOT;
    }

    int recordSize = numAttrs * ATTR_SIZE;
    unsigned char *slotPointer = bufferPtr + HEADER_SIZE + numSlots + (slotNum * recordSize);
    
    memcpy(rec, slotPointer, recordSize);
    
    return SUCCESS;
}

// Writes record to specific slot and marks block as dirty
int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) return ret;

  struct HeadInfo head;
  this->getHeader(&head);

  int numAttrs = head.numAttrs;
  int numSlots = head.numSlots;

  if (slotNum < 0 || slotNum >= numSlots) {
    return E_OUTOFBOUND;
  }

  int recordSize = numAttrs * ATTR_SIZE;
  unsigned char *slotPointer = bufferPtr + HEADER_SIZE + numSlots + (slotNum * recordSize);

  memcpy(slotPointer, rec, recordSize);

  int status = StaticBuffer::setDirtyBit(this->blockNum);
  if (status != SUCCESS) return status;

  return SUCCESS;
}

// Gets slotmap from record block
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) return ret;

  struct HeadInfo head;
  ret = getHeader(&head);
  if (ret != SUCCESS) return ret;

  int slotCount = head.numSlots;
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
  memcpy(slotMap, slotMapInBuffer, slotCount);

  return SUCCESS;
}

// Sets the slotmap of the block
int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    struct HeadInfo head;
    this->getHeader(&head);

    int numSlots = head.numSlots;
    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    return ret;
}

// Compares two Attribute values
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {
  double diff;

  if (attrType == NUMBER) {
    diff = attr1.nVal - attr2.nVal;
  } else {
    diff = strcmp(attr1.sVal, attr2.sVal);
  }

  if (diff < 0) return -1;
  if (diff > 0) return 1;
  return 0;
}