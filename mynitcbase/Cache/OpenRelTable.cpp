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
    RelCacheEntry* relCacheEntry = new RelCacheEntry();
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = RecId{RELCAT_BLOCK, RELCAT_SLOTNUM_FOR_RELCAT};
    relCacheEntry->searchIndex = RecId{-1, -1};
    relCacheEntry->dirty = false;
    RelCacheTable::relCache[RELCAT_RELID] = relCacheEntry;

    // Load RELCAT entry for ATTRIBUTECAT
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    relCacheEntry = new RelCacheEntry();
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = RecId{RELCAT_BLOCK, RELCAT_SLOTNUM_FOR_ATTRCAT};
    relCacheEntry->searchIndex = RecId{-1, -1};
    relCacheEntry->dirty = false;
    RelCacheTable::relCache[ATTRCAT_RELID] = relCacheEntry;

    // load the relation and attribute catalog into the attribute cache
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    AttrCacheEntry *head = nullptr, *tail = nullptr;

    // Load attributes for RELATIONCAT (slots 0 to 5)
    for (int slot = 0; slot < 6; ++slot) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, slot);
        AttrCacheEntry* entry = new AttrCacheEntry();
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(entry->attrCatEntry));
        entry->recId = RecId{ATTRCAT_BLOCK, slot};
        entry->dirty = false;
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
        AttrCacheEntry* entry = new AttrCacheEntry();
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(entry->attrCatEntry));
        entry->recId = RecId{ATTRCAT_BLOCK, slot};
        entry->dirty = false;
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


int OpenRelTable::getFreeOpenRelTableEntry() {
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (tableMetaInfo[i].free) {
            return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName, relName) == 0) {
            return i;
        }
    }
    return E_RELNOTOPEN;
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

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute attrVal;
    strcpy(attrVal.sVal, relName);

    char relcatAttrName[ATTR_SIZE];
    strcpy(relcatAttrName, RELCAT_ATTR_RELNAME);
    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, attrVal, EQ);

    if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBlock(relcatRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

    RelCacheEntry* relCacheEntry = new RelCacheEntry();
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId = relcatRecId;
    relCacheEntry->searchIndex = RecId{-1, -1};
    relCacheEntry->dirty = false;
    RelCacheTable::relCache[relId] = relCacheEntry;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    AttrCacheEntry *head = nullptr, *tail = nullptr;

    while (true) {
        char attrcatAttrName[ATTR_SIZE];
        strcpy(attrcatAttrName, ATTRCAT_ATTR_RELNAME);
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, attrVal, EQ);

        if (attrcatRecId.block == -1 && attrcatRecId.slot == -1) {
            break; 
        }

        RecBuffer attrCatBlock(attrcatRecId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);

        AttrCacheEntry* attrCacheEntry = new AttrCacheEntry();
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId = attrcatRecId;
        attrCacheEntry->dirty = false;
        attrCacheEntry->next = nullptr;

        if (head == nullptr) {
            head = attrCacheEntry;
            tail = attrCacheEntry;
        } else {
            tail->next = attrCacheEntry;
            tail = attrCacheEntry;
        }
    }

    AttrCacheTable::attrCache[relId] = head;
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
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

    /****** Releasing the Relation Cache entry of the relation ******/
    if (RelCacheTable::relCache[relId]->dirty == true) {
        RelCatEntry relCatBuf = RelCacheTable::relCache[relId]->relCatEntry;
        union Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatBuf, record);
        
        RecId recId = RelCacheTable::relCache[relId]->recId;
        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(record, recId.slot);
    }

    delete RelCacheTable::relCache[relId];
    RelCacheTable::relCache[relId] = nullptr;

    /****** Releasing the Attribute Cache entry of the relation ******/
    AttrCacheEntry* attrCacheNode = AttrCacheTable::attrCache[relId];
    while (attrCacheNode != nullptr) {
        AttrCacheEntry* nextNode = attrCacheNode->next;
        delete attrCacheNode;
        attrCacheNode = nextNode;
    }
    AttrCacheTable::attrCache[relId] = nullptr;

    /****** Set the Open Relation Table entry of the relation as free ******/
    tableMetaInfo[relId].free = true;

    return SUCCESS;
}
//---------------------------------stage 7----------------------------
OpenRelTable::~OpenRelTable() {
    // close every non-catalog relation that is still open
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free) {
            OpenRelTable::closeRel(i);
        }
    }

    // write back (if modified) and free the catalogs' relation cache entries
    for (int relId = ATTRCAT_RELID; relId >= RELCAT_RELID; relId--) {
        RelCacheEntry *entry = RelCacheTable::relCache[relId];
        if (entry != nullptr) {
            if (entry->dirty) {
                Attribute relCatRecord[RELCAT_NO_ATTRS];
                RelCacheTable::relCatEntryToRecord(&(entry->relCatEntry), relCatRecord);

                RecBuffer relCatBlock(entry->recId.block);
                relCatBlock.setRecord(relCatRecord, entry->recId.slot);
            }
            delete entry;
            RelCacheTable::relCache[relId] = nullptr;
        }
    }

    // free the attribute cache lists of both catalogs
    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID; relId++) {
        AttrCacheEntry *curr = AttrCacheTable::attrCache[relId];
        while (curr != nullptr) {
            AttrCacheEntry *next = curr->next;
            delete curr;
            curr = next;
        }
        AttrCacheTable::attrCache[relId] = nullptr;
    }
}
