#include "Schema.h"

#include <cmath>
#include <cstring>
//relname is the user requested relation name 
int Schema::openRel(char relName[ATTR_SIZE]) {
    //passes the relation name to cache layer for searching and loads it to memory if any slots are free using tableMetaInfo(empty, relname)
    //and returns rel-id
  int ret = OpenRelTable::openRel(relName);

  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  if(ret >= 0){
    return SUCCESS;
  }

  //otherwise it returns an error message
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
    //if rel is in catalog ??
  if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}
//stage 6 ----------------------------------------

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    // If the oldRelName or newRelName is either Relation Catalog or Attribute Catalog,
    // return E_NOTPERMITTED.
    if (strcmp(oldRelName, RELCAT_RELNAME) == 0 || 
        strcmp(oldRelName, ATTRCAT_RELNAME) == 0 ||
        strcmp(newRelName, RELCAT_RELNAME) == 0 || 
        strcmp(newRelName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    // If the relation is open, it cannot be renamed.
    if (OpenRelTable::getRelId(oldRelName) != E_RELNOTOPEN) {
        return E_RELOPEN;
    }

    // Call BlockAccess layer to update the catalogs on the disk
    int retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    
    return retVal;
}

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
    // If the relName is either Relation Catalog or Attribute Catalog,
    // return E_NOTPERMITTED.
    if (strcmp(relName, RELCAT_RELNAME) == 0 || 
        strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    // If the relation is open, its attributes cannot be renamed.
    if (OpenRelTable::getRelId(relName) != E_RELNOTOPEN) {
        return E_RELOPEN;
    }

    // Call BlockAccess layer to update the attribute catalog on the disk
    int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
    
    return retVal;
}
//--------------------------stage 7-----------------------------------
int Schema::createRel(char relName[], int nAttrs, char attrs[][ATTR_SIZE], int attrtype[]) {
  // check whether a relation with this name already exists
  Attribute relNameAsAttribute;
  strcpy(relNameAsAttribute.sVal, relName);

  RecId targetRelId;

  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  char relcatAttrName[ATTR_SIZE];
  strcpy(relcatAttrName, RELCAT_ATTR_RELNAME);   // "RelName"

  targetRelId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, relNameAsAttribute, EQ);

  if (targetRelId.block != -1 || targetRelId.slot != -1) {
    return E_RELEXIST;
  }

  // check for duplicate attribute names
  for (int i = 0; i < nAttrs; i++) {
    for (int j = i + 1; j < nAttrs; j++) {
      if (strcmp(attrs[i], attrs[j]) == 0) {
        return E_DUPLICATEATTR;
      }
    }
  }

  // build and insert the relation catalog record
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
  relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = nAttrs;
  relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
  relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
  relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
  relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = (2016 / (16 * nAttrs + 1));  // integer division = floor

  int retVal = BlockAccess::insert(RELCAT_RELID, relCatRecord);
  if (retVal != SUCCESS) {
    return retVal;
  }

  // insert one attribute catalog record per attribute
  for (int i = 0; i < nAttrs; i++) {
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    //intialise metadata 
    strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
    strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrs[i]);
    attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrtype[i];
    attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
    attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
    attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;
    //insert in metadata
    retVal = BlockAccess::insert(ATTRCAT_RELID, attrCatRecord);
    //delete the done writes if insertion failes due to space prob etc
    if (retVal != SUCCESS) {
      // roll back: remove the relcat entry (and any attrcat entries already added)
      Schema::deleteRel(relName);
      return E_DISKFULL;
    }
  }

  return SUCCESS;
}

int Schema::deleteRel(char *relName) {
  // the catalogs cannot be deleted
  if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
    return E_NOTPERMITTED;
  }

  // the relation must be closed before it is deleted
  int relId = OpenRelTable::getRelId(relName);
  if (relId >= 0 && relId < MAX_OPEN) {
    return E_RELOPEN;
  }

  return BlockAccess::deleteRelation(relName);
}