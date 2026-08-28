#include "Algebra.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
// SELECT IS INTIATED AND CONTROLLED BY ALGEBRA LAYE BUT
// SEARCH IS DNE IN BLOCK ACCESS LAYER 


//ie algbra layer finds out what the operation means and loop over the block access layer until block access layer finds the record and attrbute 
//with the help of cache layer by providing metadat like headers and searchindex
bool isNumber(char *str) {
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}
//select * from student where age > 20
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }
    //info of attribute
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if (ret != SUCCESS) {
        return E_ATTRNOTEXIST;
    }

    int type = attrCatEntry.attrType;
    Attribute attrVal;

    if (type == NUMBER) {
        if (isNumber(strVal)) {
            attrVal.nVal = atof(strVal);
        } else {
            return E_ATTRTYPEMISMATCH;
        }
    } else if (type == STRING) {
        strcpy(attrVal.sVal, strVal);
    }

    RelCacheTable::resetSearchIndex(srcRelId);
    //get relation catalog info 
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    printf("|");
    for (int i = 0; i < relCatEntry.numAttrs; ++i) {
        AttrCatEntry iterAttrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &iterAttrCatEntry);
        printf(" %s |", iterAttrCatEntry.attrName);
    }
    printf("\n");
    //search LOOP , searches using block access layer 
    while (true) {
        RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

        if (searchRes.block != -1 && searchRes.slot != -1) {
            RecBuffer recBuffer(searchRes.block);
            Attribute record[relCatEntry.numAttrs];
            recBuffer.getRecord(record, searchRes.slot);

            printf("|");
            for (int i = 0; i < relCatEntry.numAttrs; i++) {
                AttrCatEntry iterAttrCatEntry;
                AttrCacheTable::getAttrCatEntry(srcRelId, i, &iterAttrCatEntry);

                if (iterAttrCatEntry.attrType == NUMBER) {
                    printf(" %d |", (int)record[i].nVal);
                } else {
                    printf(" %s |", record[i].sVal);
                }
            }
            printf("\n");
        } else {
            break;
        }
    }

    return SUCCESS;
}