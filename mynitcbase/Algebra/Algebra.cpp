#include "Algebra.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../Cache/OpenRelTable.h"
#include "../Cache/RelCacheTable.h"
#include "../Cache/AttrCacheTable.h"
#include "../BlockAccess/BlockAccess.h"

// Helper function to check if a string represents a valid number
bool isNumber(char *str) {
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }
    
    // info of attribute
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
    
    // get relation catalog info 
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    printf("|");
    for (int i = 0; i < relCatEntry.numAttrs; ++i) {
        AttrCatEntry iterAttrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &iterAttrCatEntry);
        printf(" %s |", iterAttrCatEntry.attrName);
    }
    printf("\n");
    
    // search LOOP, searches using block access layer 
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

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]) {
    // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    // get the relation's rel-id using OpenRelTable::getRelId() method
    int relId = OpenRelTable::getRelId(relName);

    // if relation is not open in open relation table, return E_RELNOTOPEN
    if (relId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    // get the relation catalog entry from relation cache
    RelCatEntry relCatEntry;
    int retVal = RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    
    if (retVal != SUCCESS) {
        return retVal;
    }

    // if relCatEntry.numAttrs != numberOfAttributes in relation, return E_NATTRMISMATCH
    if (relCatEntry.numAttrs != nAttrs) {
        return E_NATTRMISMATCH;
    }

    // array to hold the converted record values
    union Attribute recordValues[nAttrs];

    /*
        Converting 2D char array of record values to Attribute array recordValues
    */
    for (int i = 0; i < nAttrs; i++) {
        // get the attr-cat entry for the i'th attribute from the attr-cache
        AttrCatEntry attrCatEntry;
        retVal = AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);
        
        if (retVal != SUCCESS) {
             return retVal;
        }

        int type = attrCatEntry.attrType;

        if (type == NUMBER) {
            // if the char array record[i] can be converted to a number
            if (isNumber(record[i])) {
                // convert the char array to numeral and store it
                recordValues[i].nVal = atof(record[i]);
            } else {
                return E_ATTRTYPEMISMATCH;
            }
        } else if (type == STRING) {
            // copy record[i] to recordValues[i].sVal
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    // insert the record by calling BlockAccess::insert() function
    retVal = BlockAccess::insert(relId, recordValues);

    return retVal;
}