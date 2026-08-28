#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h" // Add this line!
#include <iostream>
int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  return FrontendInterface::handleFrontend(argc, argv);
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

