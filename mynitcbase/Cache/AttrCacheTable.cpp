#include "AttrCacheTable.h"
#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS], AttrCatEntry* attrCatEntry) {
    strcpy(attrCatEntry->relName, record[0].sVal);
    strcpy(attrCatEntry->attrName, record[1].sVal);
    attrCatEntry->attrType = (int)record[2].nVal;
    attrCatEntry->primaryFlag = (bool)record[3].nVal;
    attrCatEntry->rootBlock = (int)record[4].nVal;
    attrCatEntry->offset = (int)record[5].nVal;
}

int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (attrCache[relId] == nullptr) return E_RELNOTOPEN;

    for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
        if (entry->attrCatEntry.offset == attrOffset) {
            *attrCatBuf = entry->attrCatEntry;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}


int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf) {
    if (relId < 0 || relId >= MAX_OPEN) return E_OUTOFBOUND;
    if (attrCache[relId] == nullptr) return E_RELNOTOPEN;

    for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
        if (strcmp(entry->attrCatEntry.attrName, attrName) == 0) {
            *attrCatBuf = entry->attrCatEntry;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}