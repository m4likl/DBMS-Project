#include "Algebra.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../Cache/OpenRelTable.h"
#include "../Cache/RelCacheTable.h"
#include "../Cache/AttrCacheTable.h"
#include "../BlockAccess/BlockAccess.h"
#include "../BPlusTree/BPlusTree.h"
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

  // target "null" means: print the matching records instead of creating a relation
  if (strcmp(targetRel, "null") == 0) {
    RelCatEntry srcEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcEntry);
    int nAttrs = srcEntry.numAttrs;

    int types[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
      AttrCatEntry e;
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &e);
      types[i] = e.attrType;
      printf("%s%s", e.attrName, i < nAttrs - 1 ? " | " : "\n");
    }

    RelCacheTable::resetSearchIndex(srcRelId);
    AttrCacheTable::resetSearchIndex(srcRelId, attr);
    Attribute rec[nAttrs];
    while (BlockAccess::search(srcRelId, rec, attr, attrVal, op) == SUCCESS) {
      for (int i = 0; i < nAttrs; i++) {
        if (types[i] == NUMBER) printf("%d", (int)rec[i].nVal);
        else printf("%s", rec[i].sVal);
        printf("%s", i < nAttrs - 1 ? " | " : "\n");
      }
    }
    return SUCCESS;
  }

  /*** create and open the target relation ***/
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
  int src_nAttrs = relCatEntry.numAttrs;

  char attr_names[src_nAttrs][ATTR_SIZE];
  int attr_types[src_nAttrs];
  /*** copies attributes in src relation to a new one***/
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
  /*AttrCacheTable::resetSearchIndex(srcRelId, attr);
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
  return SUCCESS;*/

    AttrCacheTable::resetSearchIndex(srcRelId, attr);
  Attribute record[src_nAttrs];

  numComparisons = 0;   // count only the comparisons made by the search itself

  while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS) {
    ret = BlockAccess::insert(targetRelId, record);
    if (ret != SUCCESS) {
      Schema::closeRel(targetRel);
      Schema::deleteRel(targetRel);
      return ret;
    }
  }

  printf("Number of comparisons: %lld\n", numComparisons);

  Schema::closeRel(targetRel);
  return SUCCESS;
}


int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
  int srcRelId = OpenRelTable::getRelId(srcRel);
  if (srcRelId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }
    // target "null" means: print every record instead of creating a relation
  if (strcmp(targetRel, "null") == 0) {
    RelCatEntry srcEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcEntry);
    int nAttrs = srcEntry.numAttrs;

    int types[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
      AttrCatEntry e;
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &e);
      types[i] = e.attrType;
      printf("%s%s", e.attrName, i < nAttrs - 1 ? " | " : "\n");
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute rec[nAttrs];
    while (BlockAccess::project(srcRelId, rec) == SUCCESS) {
      for (int i = 0; i < nAttrs; i++) {
        if (types[i] == NUMBER) printf("%d", (int)rec[i].nVal);
        else printf("%s", rec[i].sVal);
        printf("%s", i < nAttrs - 1 ? " | " : "\n");
      }
    }
    return SUCCESS;
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
int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE]) {
  int srcRelId1 = OpenRelTable::getRelId(srcRelation1);
  int srcRelId2 = OpenRelTable::getRelId(srcRelation2);

  if (srcRelId1 == E_RELNOTOPEN || srcRelId2 == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  // the join attributes must exist, one in each relation
  AttrCatEntry attrCatEntry1, attrCatEntry2;
  if (AttrCacheTable::getAttrCatEntry(srcRelId1, attribute1, &attrCatEntry1) != SUCCESS ||
      AttrCacheTable::getAttrCatEntry(srcRelId2, attribute2, &attrCatEntry2) != SUCCESS) {
    return E_ATTRNOTEXIST;
  }

  if (attrCatEntry1.attrType != attrCatEntry2.attrType) {
    return E_ATTRTYPEMISMATCH;
  }

  RelCatEntry relCatEntry1, relCatEntry2;
  RelCacheTable::getRelCatEntry(srcRelId1, &relCatEntry1);
  RelCacheTable::getRelCatEntry(srcRelId2, &relCatEntry2);

  int numOfAttributes1 = relCatEntry1.numAttrs;
  int numOfAttributes2 = relCatEntry2.numAttrs;

  // no two attributes may share a name, except the pair being joined on
  for (int i = 0; i < numOfAttributes1; i++) {
    AttrCatEntry a1;
    AttrCacheTable::getAttrCatEntry(srcRelId1, i, &a1);

    for (int j = 0; j < numOfAttributes2; j++) {
      AttrCatEntry a2;
      AttrCacheTable::getAttrCatEntry(srcRelId2, j, &a2);

      if (i == attrCatEntry1.offset && j == attrCatEntry2.offset) {
        continue;   // the join attributes themselves
      }

      if (strcmp(a1.attrName, a2.attrName) == 0) {
        return E_DUPLICATEATTR;
      }
    }
  }

  // the inner relation needs an index on its join attribute
  if (attrCatEntry2.rootBlock == -1) {
    int ret = BPlusTree::bPlusCreate(srcRelId2, attribute2);
    if (ret != SUCCESS) {
      return ret;   // in practice only E_DISKFULL
    }
  }

  // the target has every attribute of both relations except relation 2's join attribute
  int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;

  char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
  int targetRelAttrTypes[numOfAttributesInTarget];

  int k = 0;
  for (int i = 0; i < numOfAttributes1; i++) {
    AttrCatEntry a;
    AttrCacheTable::getAttrCatEntry(srcRelId1, i, &a);
    strcpy(targetRelAttrNames[k], a.attrName);
    targetRelAttrTypes[k] = a.attrType;
    k++;
  }
  for (int j = 0; j < numOfAttributes2; j++) {
    if (j == attrCatEntry2.offset) {
      continue;
    }
    AttrCatEntry a;
    AttrCacheTable::getAttrCatEntry(srcRelId2, j, &a);
    strcpy(targetRelAttrNames[k], a.attrName);
    targetRelAttrTypes[k] = a.attrType;
    k++;
  }

  int ret = Schema::createRel(targetRelation, numOfAttributesInTarget, targetRelAttrNames, targetRelAttrTypes);
  if (ret != SUCCESS) {
    return ret;
  }

  int targetRelId = OpenRelTable::openRel(targetRelation);
  if (targetRelId < 0 || targetRelId >= MAX_OPEN) {
    Schema::deleteRel(targetRelation);
    return targetRelId;
  }

  Attribute record1[numOfAttributes1];
  Attribute record2[numOfAttributes2];
  Attribute targetRecord[numOfAttributesInTarget];

  // start reading relation 1 from its first record
  RelCacheTable::resetSearchIndex(srcRelId1);

  // for every record of relation 1 ...
  while (BlockAccess::project(srcRelId1, record1) == SUCCESS) {

    // ... look up the matching records of relation 2 from the beginning
    RelCacheTable::resetSearchIndex(srcRelId2);
    AttrCacheTable::resetSearchIndex(srcRelId2, attribute2);

    while (BlockAccess::search(srcRelId2, record2, attribute2, record1[attrCatEntry1.offset], EQ) == SUCCESS) {

      // all of record1, then record2 without its join attribute
      int t = 0;
      for (int i = 0; i < numOfAttributes1; i++) {
        targetRecord[t++] = record1[i];
      }
      for (int j = 0; j < numOfAttributes2; j++) {
        if (j != attrCatEntry2.offset) {
          targetRecord[t++] = record2[j];
        }
      }

      ret = BlockAccess::insert(targetRelId, targetRecord);
      if (ret != SUCCESS) {
        OpenRelTable::closeRel(targetRelId);
        Schema::deleteRel(targetRelation);
        return E_DISKFULL;
      }
    }
  }

  OpenRelTable::closeRel(targetRelId);
  return SUCCESS;
}