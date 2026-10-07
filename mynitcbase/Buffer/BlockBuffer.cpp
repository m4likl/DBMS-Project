

#include "BlockBuffer.h"
#include <cstring>

// BlockBuffer Constructor
BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

// RecBuffer Constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}



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

//stage 6 ---------------------------

//checks if block is already in buffer
//if yes, access and timstamp=0
//else access from disk using loadblockamdgetbufferptr which uses getfreebuffer

//set dirtybit if the block changes name 

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char ** buffPtr) {
    /* check whether the block is already present in the buffer
       using StaticBuffer.getBufferNum() */
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum != E_BLOCKNOTINBUFFER) {
        // if present, set the timestamp of the corresponding buffer to 0 
        // and increment the timestamps of all other occupied buffers in BufferMetaInfo.
        for (int i = 0; i < BUFFER_CAPACITY; i++) {
            if (!StaticBuffer::metainfo[i].free) {
                StaticBuffer::metainfo[i].timeStamp++;
            }
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    } else {
        // else get a free buffer using StaticBuffer.getFreeBuffer()
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        // if the call returns E_OUTOFBOUND, return E_OUTOFBOUND here as
        // the blockNum is invalid
        if (bufferNum == E_OUTOFBOUND) {
            return E_OUTOFBOUND;
        }

        // Read the block into the free buffer using readBlock()
        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }

    // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}


//finds the record needed using eqn 
//memcpy the data to buffer
//sets dirtybit to identify changes 
int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    if (ret != SUCCESS) {
        return ret;
    }

    /* get the header of the block using the getHeader() function */
    struct HeadInfo head;
    this->getHeader(&head);

    // get number of attributes in the block.
    int numAttrs = head.numAttrs;

    // get the number of slots in the block.
    int numSlots = head.numSlots;

    // if input slotNum is not in the permitted range return E_OUTOFBOUND.
    if (slotNum < 0 || slotNum >= numSlots) {
        return E_OUTOFBOUND;
    }

    /* offset bufferPtr to point to the beginning of the record at required
       slot. the block contains the header, the slotmap, followed by all
       the records.
    */
    int recordSize = numAttrs * ATTR_SIZE;
    
    // Pointer math: Header (32 bytes) + Slotmap size (numSlots bytes) + offset for previous records
    unsigned char *slotPointer = bufferPtr + HEADER_SIZE + numSlots + (slotNum * recordSize);

    // copy the record from `rec` to buffer using memcpy
    memcpy(slotPointer, rec, recordSize);

    // update dirty bit using setDirtyBit()
    StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}