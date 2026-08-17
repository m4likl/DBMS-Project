#include "Disk_Class/Disk.h"
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Cache/RelCacheTable.h"
#include "Cache/AttrCacheTable.h"
#include "define/constants.h"
#include <cstdio>
#include <iostream>


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
