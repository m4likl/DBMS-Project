#include "Disk_Class/Disk.h"
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Cache/RelCacheTable.h"
#include "Cache/AttrCacheTable.h"
#include "define/constants.h"
#include <cstdio>
#include <iostream>

/*-----LAB-----*/
int main(int argc, char *argv[]) {
    int relId = OpenRelTable::getRelId((char*)"RELATIONCAT");

    
        std::cout << "id:" << relId << std::endl;

    return 0;
}
/*
int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  // Iterate over relId 0 (RELCAT), 1 (ATTRCAT), and 2 (Students)
  for (int i = 0; i <= 2; i++) {
    
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(i, &relCatEntry);

    printf("Relation: %s\n", relCatEntry.relName);

    for (int j = 0; j < relCatEntry.numAttrs; j++) {
      AttrCatEntry attrCatEntry;
      AttrCacheTable::getAttrCatEntry(i, j, &attrCatEntry);

      const char *attrType = (attrCatEntry.attrType == NUMBER) ? "NUM" : "STR";

      printf("  %s: %s\n", attrCatEntry.attrName, attrType);
    }
    printf("\n");
  }
  
  return 0;
}
*/
/*

  ----STAGE 3---- ACCESSING THROUGH STATIC BUFFER
#include "Disk_Class/Disk.h"
#include "Buffer/StaticBuffer.h"
#include "Buffer/BlockBuffer.h"
#include "define/constants.h"
#include <iostream>
#include <cstring>
#include <cstdio>

int main(int argc, char *argv[]) {
  // Initialize Disk and StaticBuffer objects
  // (StaticBuffer MUST be declared after Disk)
  Disk disk_run;
  StaticBuffer buffer;

  // Create RecBuffer instances for Relation Catalog and Attribute Catalog blocks
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  // Load headers using the buffer mechanism
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  // Iterate over all relations in the Relation Catalog
  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // Iterate over all attributes in the Attribute Catalog
    for (int j = 0; j < attrCatHeader.numEntries; j++) {

      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord, j);

      // Print attributes belonging to the current relation
      if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
        const char *attrType = ((int)attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER) ? "NUM" : "STR";
        printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
      }
    }
    printf("\n");
  }

  return 0;
}
*/
/*

EDITING CODE FROM BUFFER AND UPDATING WITHOUT GET/SER BUFFER  ---STAGE 2---
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include <cstring>
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;

  RecBuffer relCatBuffer(RELCAT_BLOCK);

  HeadInfo relCatHeader;

  relCatBuffer.getHeader(&relCatHeader);

  bool renamed = false;
  int blockNum = ATTRCAT_BLOCK;

  while (blockNum != -1 && !renamed) {

    RecBuffer attrCatBuffer(blockNum);

    struct HeadInfo attrBlockHeader;
    attrCatBuffer.getHeader(&attrBlockHeader);

    for (
        int slot = 0;
        slot < attrBlockHeader.numEntries;
        slot++
    ) {

      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

      attrCatBuffer.getRecord(attrCatRecord, slot);

      bool isStudentsRelation =
          strcmp(
              attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,
              "Paticipants"
          ) == 0;

      bool isClassAttribute =
          strcmp(
              attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
              "event"
          ) == 0;

      if (isStudentsRelation && isClassAttribute) {

        memset(
            attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
            0,
            ATTR_SIZE
        );

        strcpy(
            attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
            "Name"
        );

        unsigned char buffer[BLOCK_SIZE];

        Disk::readBlock(buffer, blockNum);

        int recordSize =
            attrBlockHeader.numAttrs * ATTR_SIZE;

        unsigned char *slotPointer =
            buffer
            + HEADER_SIZE
            + attrBlockHeader.numSlots
            + (slot * recordSize);

        memcpy(
            slotPointer,
            attrCatRecord,
            recordSize
        );

        Disk::writeBlock(buffer, blockNum);

        renamed = true;
        break;
      }
    }

    if (!renamed) {
      blockNum = attrBlockHeader.rblock;
    }
  }

  if (renamed) {
    printf(
        "Attribute Paticipants.Class renamed to Name successfully.\n\n"
    );
  } else {
    printf(
        "Attribute Paticipants.Class was not found.\n\n"
    );
  }

  printf("Schema after modification:\n\n");

  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBuffer.getRecord(relCatRecord, i);

    printf(
        "Relation: %s\n",
        relCatRecord[RELCAT_REL_NAME_INDEX].sVal
    );

    int currentBlock = ATTRCAT_BLOCK;

    while (currentBlock != -1) {

      RecBuffer attrCatBuffer(currentBlock);

      struct HeadInfo attrBlockHeader;
      attrCatBuffer.getHeader(&attrBlockHeader);

      for (
          int slot = 0;
          slot < attrBlockHeader.numEntries;
          slot++
      ) {

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

        attrCatBuffer.getRecord(attrCatRecord, slot);

        if (strcmp(
                attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,
                relCatRecord[RELCAT_REL_NAME_INDEX].sVal
            ) == 0) {

          const char *attrType =
              attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER
                  ? "NUM"
                  : "STR";

          printf(
              "  %s: %s\n",
              attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
              attrType
          );
        }
      }

      currentBlock = attrBlockHeader.rblock;
    }

    printf("\n");
  }

  return 0;
}

prints block allocation maps 


#include "Disk_Class/Disk.h"
#include "define/constants.h"
#include <iostream>
int main() {
  Disk disk_run;
  unsigned char buffer[BLOCK_SIZE];

  for (int map = 0; map < 4; map++) {
    Disk::readBlock(buffer, map);

    for (int offset = 0; offset < BLOCK_SIZE; offset++) {
      int blockNum = (map * BLOCK_SIZE) + offset;
      unsigned char blockType = buffer[offset];

      //if (blockType != UNUSED_BLK) {
        std::cout  << (int)blockType << ", ";
      //}
    }
  }

  return 0;
}

*/


