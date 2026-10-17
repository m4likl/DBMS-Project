#include "RelCacheTable.h"
#include <cstring>

// Array of pointers representing the Relation Cache
RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

// Converts a relation catalog record from buffer/disk format into struct RelCatEntry
void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry) {
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal; 
}

// Converts a struct RelCatEntry back into a record array format for writing to disk
void RelCacheTable::relCatEntryToRecord(RelCatEntry *relCatEntry, union Attribute record[RELCAT_NO_ATTRS]) {
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, relCatEntry->relName);
    record[RELCAT_NO_ATTRIBUTES_INDEX].nVal = (double)relCatEntry->numAttrs;
    record[RELCAT_NO_RECORDS_INDEX].nVal = (double)relCatEntry->numRecs;
    record[RELCAT_FIRST_BLOCK_INDEX].nVal = (double)relCatEntry->firstBlk;
    record[RELCAT_LAST_BLOCK_INDEX].nVal = (double)relCatEntry->lastBlk;
    record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = (double)relCatEntry->numSlotsPerBlk;
}

// Retrieves relation catalog entry from the cache
int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatBuf) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;

    *relCatBuf = relCache[relId]->relCatEntry;
    return SUCCESS;
}

// Updates relation catalog entry in the cache and marks it as dirty
int RelCacheTable::setRelCatEntry(int relId, RelCatEntry *relCatBuf) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;

    relCache[relId]->relCatEntry = *relCatBuf;
    relCache[relId]->dirty = true;

    return SUCCESS;
}

// Gets search index for a relation
int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;

    *searchIndex = relCache[relId]->searchIndex;
    return SUCCESS;
}

// Sets search index for a relation
int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;

    relCache[relId]->searchIndex = *searchIndex;
    return SUCCESS;
}

// Resets search index to {-1, -1}
int RelCacheTable::resetSearchIndex(int relId) {
    RecId tempSearchIndex = {-1, -1};
    return setSearchIndex(relId, &tempSearchIndex);
}