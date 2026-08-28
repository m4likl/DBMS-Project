#include "OpenRelTable.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
//--------------------------------------------------------------------
//OpenRelTable reading catalog records and turning them into cache entries
OpenRelTable::OpenRelTable() {
    // Initialize all cache entries to nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    // Load RELCAT entry for RELATIONCAT
    int ret = relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
    if (ret != SUCCESS) {
        printf("Error: Failed to read RELCAT entry for RELATIONCAT. Disk may be empty or unformatted.\n");
        exit(1);
    }

    RelCacheEntry* relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = RecId{RELCAT_BLOCK, RELCAT_SLOTNUM_FOR_RELCAT};
    relCacheEntry->searchIndex = RecId{-1, -1};
    RelCacheTable::relCache[RELCAT_RELID] = relCacheEntry;

    // Load RELCAT entry for ATTRIBUTECAT
    ret = relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    if (ret != SUCCESS) {
        printf("Error: Failed to read RELCAT entry for ATTRIBUTECAT.\n");
        exit(1);
    }
//MADE THE RAW DATA IN DISK TO RELCATENTRY WHICH IS READABLE FOR    
    relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = RecId{RELCAT_BLOCK, RELCAT_SLOTNUM_FOR_ATTRCAT};
    relCacheEntry->searchIndex = RecId{-1, -1};
    RelCacheTable::relCache[ATTRCAT_RELID] = relCacheEntry;

    // Load attributes for RELATIONCAT and ATTRIBUTECAT
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    AttrCacheEntry *head = nullptr, *tail = nullptr;

    // Load attributes for RELATIONCAT (slots 0 to 5)
    for (int slot = 0; slot < 6; ++slot) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        ret = attrCatBlock.getRecord(attrCatRecord, slot);
        if (ret != SUCCESS) {
            printf("Error: Failed to read ATTRCAT entry at slot %d.\n", slot);
            exit(1);
        }

        AttrCacheEntry* entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(entry->attrCatEntry));
        entry->recId = RecId{ATTRCAT_BLOCK, slot};
        entry->next = nullptr;

        if (head == nullptr) {
            head = entry;
            tail = entry;
        } else {
            tail->next = entry;
            tail = entry;
        }
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    head = nullptr;
    tail = nullptr;

    // Load attributes for ATTRIBUTECAT (slots 6 to 11)
    for (int slot = 6; slot < 12; ++slot) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        ret = attrCatBlock.getRecord(attrCatRecord, slot);
        if (ret != SUCCESS) {
            printf("Error: Failed to read ATTRCAT entry at slot %d.\n", slot);
            exit(1);
        }

        AttrCacheEntry* entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(entry->attrCatEntry));
        entry->recId = RecId{ATTRCAT_BLOCK, slot};
        entry->next = nullptr;

        if (head == nullptr) {
            head = entry;
            tail = entry;
        } else {
            tail->next = entry;
            tail = entry;
        }
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;
}

OpenRelTable::~OpenRelTable() {
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (RelCacheTable::relCache[i] != nullptr) {
            free(RelCacheTable::relCache[i]);
        }
        AttrCacheEntry* curr = AttrCacheTable::attrCache[i];
        while (curr != nullptr) {
            AttrCacheEntry* next = curr->next;
            free(curr);
            curr = next;
        }
    }
}


int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    // Hardcoded for Stage 4 implementation
    if (strcmp(relName, RELCAT_RELNAME) == 0) return RELCAT_RELID;
    if (strcmp(relName, ATTRCAT_RELNAME) == 0) return ATTRCAT_RELID;
    
    return E_RELNOTOPEN;
}