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

  AttrCatEntry attrCatEntry;
  if (AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry) != SUCCESS) {
    return E_ATTRNOTEXIST;
  }

  /*** convert strVal to NUMBER or STRING ***/
  Attribute attrVal;
  int type = attrCatEntry.attrType;

  if (type == NUMBER) {
    if (isNumber(strVal)) {
      attrVal.nVal = atof(strVal);
    } else {
      return E_ATTRTYPEMISMATCH;
    }
  } else if (type == STRING) {
    strcpy(attrVal.sVal, strVal);
  }

  /*** create and open the target relation ***/
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
  int src_nAttrs = relCatEntry.numAttrs;

  char attr_names[src_nAttrs][ATTR_SIZE];
  int attr_types[src_nAttrs];

  for (int i = 0; i < src_nAttrs; i++) {
    AttrCatEntry entry;
    AttrCacheTable::getAttrCatEntry(srcRelId, i, &entry);
    strcpy(attr_names[i], entry.attrName);
    attr_types[i] = entry.attrType;
  }

  int ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
  if (ret != SUCCESS) {
    return ret;
  }

  int targetRelId = OpenRelTable::openRel(targetRel);
  if (targetRelId < 0 || targetRelId >= MAX_OPEN) {
    Schema::deleteRel(targetRel);
    return targetRelId;
  }

  /*** select records and insert them into the target relation ***/
  RelCacheTable::resetSearchIndex(srcRelId);
  // (AttrCacheTable::resetSearchIndex is only needed once B+ trees exist, so it is omitted)

  Attribute record[src_nAttrs];

  while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS) {
    ret = BlockAccess::insert(targetRelId, record);
    if (ret != SUCCESS) {
      Schema::closeRel(targetRel);
      Schema::deleteRel(targetRel);
      return ret;
    }
  }

  Schema::closeRel(targetRel);
  return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
  int srcRelId = OpenRelTable::getRelId(srcRel);
  if (srcRelId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
  int numAttrs = relCatEntry.numAttrs;

  char attrNames[numAttrs][ATTR_SIZE];
  int attrTypes[numAttrs];

  for (int i = 0; i < numAttrs; i++) {
    AttrCatEntry entry;
    AttrCacheTable::getAttrCatEntry(srcRelId, i, &entry);
    strcpy(attrNames[i], entry.attrName);
    attrTypes[i] = entry.attrType;
  }

  /*** create and open the target relation ***/
  int ret = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);
  if (ret != SUCCESS) {
    return ret;
  }

  int targetRelId = OpenRelTable::openRel(targetRel);
  if (targetRelId < 0 || targetRelId >= MAX_OPEN) {
    Schema::deleteRel(targetRel);
    return targetRelId;
  }

  /*** copy every record ***/
  RelCacheTable::resetSearchIndex(srcRelId);

  Attribute record[numAttrs];

  while (BlockAccess::project(srcRelId, record) == SUCCESS) {
    ret = BlockAccess::insert(targetRelId, record);
    if (ret != SUCCESS) {
      Schema::closeRel(targetRel);
      Schema::deleteRel(targetRel);
      return ret;
    }
  }

  Schema::closeRel(targetRel);
  return SUCCESS;
}
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
  int srcRelId = OpenRelTable::getRelId(srcRel);
  if (srcRelId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
  int src_nAttrs = relCatEntry.numAttrs;

  // offset and type (in the source relation) of each projected attribute
  int attr_offset[tar_nAttrs];
  int attr_types[tar_nAttrs];

  for (int i = 0; i < tar_nAttrs; i++) {
    AttrCatEntry entry;
    if (AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &entry) != SUCCESS) {
      return E_ATTRNOTEXIST;
    }
    attr_offset[i] = entry.offset;
    attr_types[i] = entry.attrType;
  }

  /*** create and open the target relation ***/
  int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);
  if (ret != SUCCESS) {
    return ret;
  }

  int targetRelId = OpenRelTable::openRel(targetRel);
  if (targetRelId < 0 || targetRelId >= MAX_OPEN) {
    Schema::deleteRel(targetRel);
    return targetRelId;
  }

  /*** insert the projected records ***/
  RelCacheTable::resetSearchIndex(srcRelId);

  Attribute record[src_nAttrs];

  while (BlockAccess::project(srcRelId, record) == SUCCESS) {
    Attribute proj_record[tar_nAttrs];

    for (int i = 0; i < tar_nAttrs; i++) {
      proj_record[i] = record[attr_offset[i]];
    }

    ret = BlockAccess::insert(targetRelId, proj_record);
    if (ret != SUCCESS) {
      Schema::closeRel(targetRel);
      Schema::deleteRel(targetRel);
      return ret;
    }
  }

  Schema::closeRel(targetRel);
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