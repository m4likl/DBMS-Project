#include "OpenRelTable.h"
#include "RelCacheTable.h"
#include "AttrCacheTable.h"
#include "../Buffer/BlockBuffer.h"
#include "../define/constants.h"
#include <cstdlib>
#include <cstring>
OpenRelTable::OpenRelTable() {

  // Initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  /*----------- Setting up Relation Cache entries -------------*/

  RecBuffer relCatBlock(RELCAT_BLOCK);
  Attribute relCatRecord[RELCAT_NO_ATTRS];

  /**** 1. Relation Catalog (relId = 0) ****/
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** 2. Attribute Catalog (relId = 1) ****/
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

  /**** 3. Students Relation (relId = 2) ****/
  relCatBlock.getRecord(relCatRecord, 2); // Slot 2 in RELATIONCAT block
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = 2;

  RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[2]) = relCacheEntry;


  /************ Setting up Attribute Cache entries ************/

  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  AttrCacheEntry *head = nullptr;
  AttrCacheEntry *tail = nullptr;

  /**** 1. Relation Catalog Attributes (relId = 0, slots 0-5) ****/
  for (int i = 0; i < RELCAT_NO_ATTRS; ++i) {
    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheEntry *entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &entry->attrCatEntry);
    entry->recId.block = ATTRCAT_BLOCK;
    entry->recId.slot = i;
    entry->next = nullptr;

    if (head == nullptr) { head = entry; tail = entry; }
    else { tail->next = entry; tail = entry; }
  }
  AttrCacheTable::attrCache[RELCAT_RELID] = head;

  /**** 2. Attribute Catalog Attributes (relId = 1, slots 6-11) ****/
  head = nullptr;
  tail = nullptr;
  for (int i = RELCAT_NO_ATTRS; i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; ++i) {
    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheEntry *entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &entry->attrCatEntry);
    entry->recId.block = ATTRCAT_BLOCK;
    entry->recId.slot = i;
    entry->next = nullptr;

    if (head == nullptr) { head = entry; tail = entry; }
    else { tail->next = entry; tail = entry; }
  }
  AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

  /**** 3. Students Relation Attributes (relId = 2, slots 12+) ****/
  head = nullptr;
  tail = nullptr;
  int studentsNumAttrs = RelCacheTable::relCache[2]->relCatEntry.numAttrs;
  int studentsAttrStartSlot = RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; // 6 + 6 = 12

  for (int i = studentsAttrStartSlot; i < studentsAttrStartSlot + studentsNumAttrs; ++i) {
    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheEntry *entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &entry->attrCatEntry);
    entry->recId.block = ATTRCAT_BLOCK;
    entry->recId.slot = i;
    entry->next = nullptr;

    if (head == nullptr) { head = entry; tail = entry; }
    else { tail->next = entry; tail = entry; }
  }
  AttrCacheTable::attrCache[2] = head;
}



int OpenRelTable::getRelId( char relName[ATTR_SIZE]) {
    for (int relId = 0; relId < MAX_OPEN; relId++) {
        RelCatEntry relCatBuf;
        int status = RelCacheTable::getRelCatEntry(relId, &relCatBuf);
        if (status = true &&  strcmp((char*)relCatBuf.relName, relName) == 0) {
            return relId;
        }
    }
    return E_RELNOTOPEN;
}

OpenRelTable::~OpenRelTable() {
  for (int i = 0; i < MAX_OPEN; ++i) {
    if (RelCacheTable::relCache[i] != nullptr) {
      free(RelCacheTable::relCache[i]);
      RelCacheTable::relCache[i] = nullptr;
    }
    if (AttrCacheTable::attrCache[i] != nullptr) {
      AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
      while (curr != nullptr) {
        AttrCacheEntry *next = curr->next;
        free(curr);
        curr = next;
      }
      AttrCacheTable::attrCache[i] = nullptr;
    }
  }
}