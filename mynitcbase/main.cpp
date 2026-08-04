#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstring>

int main(int argc, char *argv[]) {
  Disk disk_run;

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);
  //iterate through all no of relation entries 
  for (int i=0; i<relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    for (int j=0; j<attrCatHeader.numEntries; j++) {

      // declare attrCatRecord and load the attribute catalog entry into it
      // 3. Declare attrCatRecord and load the attribute catalog entry into it
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord, j);
      if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
        const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        printf("  %s: %s\n", attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, attrType);
      }
    }
    printf("\n");
  }

  return 0;
}

//int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  //Disk disk_run;
  // StaticBuffer buffer;
  //input and write to disk using buffer
  //unsigned char buffer[BLOCK_SIZE];
  //Disk::readBlock(buffer, 7000);
  //char message[]="hello";
  //memcpy(buffer+20, message, 6);
  //Disk::writeBlock(buffer, 7000);

  //read from disk into a new buffer
  //unsigned char buffer2[BLOCK_SIZE];
  //char message2[6];
  //Disk::readBlock(buffer2, 7000);
  //memcpy(message2, buffer2 + 20, 6);
 // std::cout << message2;
  //commmented out handle frontend
  //return FrontendInterface::handleFrontend(argc, argv);
//} 


