#include "RelCacheTable.h"
#include <cstring>
// THE SERACH INDEX IS UPDATED AND STORED USING RELCACHE
// RELATION INFO IS RETURNED WHEN REQUESTED BY BLOCKACCESS LAYER
RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry) {
    strcpy(relCatEntry->relName, record[0].sVal);
    relCatEntry->numAttrs = (int)record[1].nVal;
    relCatEntry->numRecs = (int)record[2].nVal;
    relCatEntry->firstBlk = (int)record[3].nVal;
    relCatEntry->lastBlk = (int)record[4].nVal;
    
    // FIX: The struct member is numSlotsPerBlk, not numSlots
    relCatEntry->numSlotsPerBlk = (int)record[5].nVal; 
}

int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatBuf) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;
    *relCatBuf = relCache[relId]->relCatEntry;
    return SUCCESS;
}


int RelCacheTable::setRelCatEntry(int relId, RelCatEntry* relCatBuf) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;
    relCache[relId]->relCatEntry = *relCatBuf;
    return SUCCESS;
}

int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;
    *searchIndex = relCache[relId]->searchIndex;
    return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (relCache[relId] == nullptr) return E_RELNOTOPEN;
    relCache[relId]->searchIndex = *searchIndex;
    return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId) {
    RecId tempSearchIndex = {-1, -1};
    return setSearchIndex(relId, &tempSearchIndex);
}