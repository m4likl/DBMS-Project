#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include "define/constants.h"
#include <cstdio>
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  return FrontendInterface::handleFrontend(argc, argv);
}

/*
int main(int argc, char *argv[]) {
  Disk disk_run;
  int ret;

  {
    StaticBuffer buffer;
    OpenRelTable cache;

    ret = FrontendInterface::handleFrontend(argc, argv);
  }  // cache and buffer are destroyed here, which writes everything back to disk

  // read the block allocation map (disk blocks 0-3) and print the used blocks
  unsigned char bmap[DISK_BLOCKS];
  for (int b = 0; b < 4; b++) {
    Disk::readBlock(bmap + b * BLOCK_SIZE, b);
  }

  printf("\n--- block allocation map ---\n");
  int used = 0;
  for (int i = 0; i < DISK_BLOCKS; i++) {
    if (bmap[i] != UNUSED_BLK) {
      printf("block %d : type %d\n", i, (int)bmap[i]);
      used++;
    }
  }
  printf("%d of %d blocks in use\n", used, DISK_BLOCKS);

  return ret;
}
*/

/*
#include "Buffer/StaticBuffer.h"
#include "Buffer/BlockBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include "define/constants.h"
#include <cstdio>
#include <cstring>
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  int ret;

  {
    StaticBuffer buffer;
    OpenRelTable cache;

    ret = FrontendInterface::handleFrontend(argc, argv);
  }  // destructors run here and write everything back to disk

  // load the block allocation map (disk blocks 0-3) to find the record blocks
  unsigned char bmap[DISK_BLOCKS];
  for (int b = 0; b < 4; b++) {
    Disk::readBlock(bmap + b * BLOCK_SIZE, b);
  }

  printf("\n--- slot maps (1 = occupied, 0 = free) ---\n");
  for (int i = 0; i < DISK_BLOCKS; i++) {
    if (bmap[i] != REC) continue;

    unsigned char block[BLOCK_SIZE];
    Disk::readBlock(block, i);

    HeadInfo head;
    memcpy(&head, block, sizeof(HeadInfo));

    unsigned char *slotMap = block + HEADER_SIZE;

    printf("block %d: entries %d/%d, lblock %d, rblock %d\n  ",
           i, head.numEntries, head.numSlots, head.lblock, head.rblock);
    for (int s = 0; s < head.numSlots; s++) {
      printf("%d", slotMap[s] == SLOT_OCCUPIED ? 1 : 0);
    }
    printf("\n");
  }

  return ret;
}*/
