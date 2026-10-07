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
