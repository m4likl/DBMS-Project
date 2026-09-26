#include "OpenRelTable.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {
    // initialize all values in relCache and attrCache to be nullptr and all entries
    // in tableMetaInfo to be free
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        tableMetaInfo[i].free = true;
    }

    // load the relation and attribute catalog into the relation cache
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    // Load RELCAT entry for RELATIONCAT
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
    RelCacheEntry* relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = RecId{RELCAT_BLOCK, RELCAT_SLOTNUM_FOR_RELCAT};
    relCacheEntry->searchIndex = RecId{-1, -1};
    RelCacheTable::relCache[RELCAT_RELID] = relCacheEntry;

    // Load RELCAT entry for ATTRIBUTECAT
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = RecId{RELCAT_BLOCK, RELCAT_SLOTNUM_FOR_ATTRCAT};
    relCacheEntry->searchIndex = RecId{-1, -1};
    RelCacheTable::relCache[ATTRCAT_RELID] = relCacheEntry;

    // load the relation and attribute catalog into the attribute cache
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    AttrCacheEntry *head = nullptr, *tail = nullptr;

    // Load attributes for RELATIONCAT (slots 0 to 5)
    for (int slot = 0; slot < 6; ++slot) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, slot);
        AttrCacheEntry* entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(entry->attrCatEntry));
        entry->recId = RecId{ATTRCAT_BLOCK, slot};
        entry->next = nullptr;
        if (head == nullptr) { head = entry; tail = entry; } 
        else { tail->next = entry; tail = entry; }
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    head = nullptr;
    tail = nullptr;

    // Load attributes for ATTRIBUTECAT (slots 6 to 11)
    for (int slot = 6; slot < 12; ++slot) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, slot);
        AttrCacheEntry* entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(entry->attrCatEntry));
        entry->recId = RecId{ATTRCAT_BLOCK, slot};
        entry->next = nullptr;
        if (head == nullptr) { head = entry; tail = entry; } 
        else { tail->next = entry; tail = entry; }
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    /************ Setting up tableMetaInfo entries ************/
    tableMetaInfo[RELCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);

    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable() {
    // close all open relations (from rel-id = 2 onwards. Why? Because 0 and 1 are system catalogs)
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free) {
            OpenRelTable::closeRel(i);
        }
    }

    // free the memory allocated for rel-id 0 and 1 in the caches
    for (int i = 0; i < 2; ++i) {
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

int OpenRelTable::getFreeOpenRelTableEntry() {
    // traverse through the tableMetaInfo array, find a free entry in the Open Relation Table.
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (tableMetaInfo[i].free) {
            return i;
        }
    }
    // if not found return E_CACHEFULL.
    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    // traverse through the tableMetaInfo array, find the entry corresponding to relName.
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName, relName) == 0) {
            return i;
        }
    }
    // if not found return E_RELNOTOPEN.
    return E_RELNOTOPEN;
}

int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
        return E_NOTPERMITTED;
    }

    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    if (tableMetaInfo[relId].free) {
        return E_RELNOTOPEN;
    }

    // free the memory allocated in the relation cache
    if (RelCacheTable::relCache[relId] != nullptr) {
        free(RelCacheTable::relCache[relId]);
        RelCacheTable::relCache[relId] = nullptr;
    }

    // free the memory allocated in the attribute cache
    AttrCacheEntry* curr = AttrCacheTable::attrCache[relId];
    while (curr != nullptr) {
        AttrCacheEntry* next = curr->next;
        free(curr);
        curr = next;
    }
    AttrCacheTable::attrCache[relId] = nullptr;

    // update `tableMetaInfo` to set `relId` as a free slot
    tableMetaInfo[relId].free = true;

    return SUCCESS;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
    int relId = getRelId(relName);
    if(relId >= 0){
        return relId; // Relation is already open
    }

    relId = getFreeOpenRelTableEntry();
    if (relId == E_CACHEFULL) {
        return E_CACHEFULL;
    }

    // reset the searchIndex of the relation catalog before calling linearSearch()
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute attrVal;
    strcpy(attrVal.sVal, relName);

    // Search the relation catalog for the given relation name
    char relcatAttrName[ATTR_SIZE];
    strcpy(relcatAttrName, RELCAT_ATTR_RELNAME);
    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, attrVal, EQ);

    if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    // Read the record from the relation catalog block
    RecBuffer relCatBlock(relcatRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

    // Populate RelCacheEntry
    RelCacheEntry* relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = relcatRecId;
    relCacheEntry->searchIndex = RecId{-1, -1};
    RelCacheTable::relCache[relId] = relCacheEntry;

    // Reset the searchIndex of the attribute catalog before searching for attributes
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    AttrCacheEntry *head = nullptr, *tail = nullptr;

    // Fetch all attributes for this relation from the Attribute Catalog
    while (true) {
        char attrcatAttrName[ATTR_SIZE];
        strcpy(attrcatAttrName, ATTRCAT_ATTR_RELNAME);
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, attrVal, EQ);

        if (attrcatRecId.block == -1 && attrcatRecId.slot == -1) {
            break; // No more attributes found
        }

        RecBuffer attrCatBlock(attrcatRecId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);

        AttrCacheEntry* attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId = attrcatRecId;
        attrCacheEntry->next = nullptr;

        if (head == nullptr) {
            head = attrCacheEntry;
            tail = attrCacheEntry;
        } else {
            tail->next = attrCacheEntry;
            tail = attrCacheEntry;
        }
    }

    // Attach the linked list to the attribute cache
    AttrCacheTable::attrCache[relId] = head;

    // Update tableMetaInfo
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}