#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstring>

int main(int argc, char *argv[]) {//what was this 
  Disk disk_run;

  // Buffer for Relation Catalog (Block 4)
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  HeadInfo relCatHeader;
  relCatBuffer.getHeader(&relCatHeader);

  // Loop through all relations in Relation Catalog
  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);// to get the record from teh disk and use setrecord to save changes

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // Start reading Attribute Catalog from the first block (Block 5)
    int currentAttrBlock = ATTRCAT_BLOCK;

    // Traverse all linked attribute blocks (rblock chain)
    while (currentAttrBlock != -1) {
      RecBuffer attrCatBuffer(currentAttrBlock);
      HeadInfo attrCatHeader;
      attrCatBuffer.getHeader(&attrCatHeader);

      for (int j = 0; j < attrCatHeader.numEntries; j++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);
      
        if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, "Students")==0 && strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class")==0) {
          strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch");
          //attrCatBuffer.setRecord(attrCatRecord, j);
        }

      
        // Check if attribute belongs to the current relation
        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }

      // Move to the next linked block in the Attribute Catalog chain
      currentAttrBlock = attrCatHeader.rblock;
    }

    printf("\n");
  }

  return 0;
}