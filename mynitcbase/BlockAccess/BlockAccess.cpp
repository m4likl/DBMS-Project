#include "BlockAccess.h"
#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1) {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        block = relCatEntry.firstBlk;
        slot = 0;
    } else {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    AttrCatEntry attrCatEntry;
    int attrRes = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (attrRes != SUCCESS) {
        return RecId{-1, -1};
    }

    int attrOffset = attrCatEntry.offset;
    int attrType = attrCatEntry.attrType;

    while (block != -1) {
        RecBuffer recBuffer(block);

        HeadInfo head;
        recBuffer.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        if (slot >= head.numSlots) {
            block = head.rblock;
            slot = 0;
            continue;
        }

        if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
            continue;
        }

        Attribute record[head.numAttrs];
        recBuffer.getRecord(record, slot);

        int cmpVal = compareAttrs(record[attrOffset], attrVal, attrType);

        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0)
        ) {
            RecId searchIndex = {block, slot};
            RelCacheTable::setSearchIndex(relId, &searchIndex);
            return searchIndex;
        }

        slot++;
    }

    return RecId{-1, -1};
}
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    RecId recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);

    if (recId.block == -1 && recId.slot == -1) {
        return E_NOTFOUND;
    }

    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(record, recId.slot);

    return SUCCESS;
}
//stage 6-------------------------------------------

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal, newName);

    // search the relation catalog for an entry with "RelName" = newRelationName
    char relNameAttrStr[ATTR_SIZE] = "RelName";
    RecId searchRes = linearSearch(RELCAT_RELID, relNameAttrStr, newRelationName, EQ);

    // If relation with name newName already exists (result of linearSearch is not {-1, -1})
    if (searchRes.block != -1 && searchRes.slot != -1) {
        return E_RELEXIST;
    }

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
    searchRes = linearSearch(RELCAT_RELID, relNameAttrStr, oldRelationName, EQ);

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    if (searchRes.block == -1 && searchRes.slot == -1) {
        return E_RELNOTEXIST;
    }

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    RecBuffer relCatBuffer(searchRes.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, searchRes.slot);

    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, newName);

    // set back the record value using RecBuffer.setRecord
    relCatBuffer.setRecord(relCatRecord, searchRes.slot);


    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    // Fetch the number of attributes to know how many entries to update in ATTRCAT
    int numAttrs = relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for (int i = 0; i < numAttrs; i++) {
        // linearSearch on the attribute catalog for relName = oldRelationName
        RecId attrSearchRes = linearSearch(ATTRCAT_RELID, relNameAttrStr, oldRelationName, EQ);
        
        // get the record using RecBuffer.getRecord
        RecBuffer attrCatBuffer(attrSearchRes.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, attrSearchRes.slot);

        // update the relName field in the record to newName
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        
        // set back the record using RecBuffer.setRecord
        attrCatBuffer.setRecord(attrCatRecord, attrSearchRes.slot);
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal, relName);

    // Search for the relation with name relName in relation catalog using linearSearch()
    char relNameAttrStr[ATTR_SIZE] = "RelName";
    RecId searchRes = linearSearch(RELCAT_RELID, relNameAttrStr, relNameAttr, EQ);

    // If relation with name relName does not exist (search returns {-1,-1})
    if (searchRes.block == -1 && searchRes.slot == -1) {
        return E_RELNOTEXIST;
    }

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
        searchRes = linearSearch(ATTRCAT_RELID, relNameAttrStr, relNameAttr, EQ);

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        if (searchRes.block == -1 && searchRes.slot == -1) {
            break;
        }

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        RecBuffer attrCatBuffer(searchRes.block);
        attrCatBuffer.getRecord(attrCatEntryRecord, searchRes.slot);

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0) {
            attrToRenameRecId = searchRes;
        }

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0) {
            return E_ATTREXIST;
        }
    }

    // if attrToRenameRecId == {-1, -1}
    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) {
        return E_ATTRNOTEXIST;
    }


    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /* declare a RecBuffer for attrToRenameRecId.block and get the record at
       attrToRenameRecId.slot */
    RecBuffer attrCatBuffer(attrToRenameRecId.block);
    attrCatBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    // update the AttrName of the record with newName
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);

    // set back the record with RecBuffer.setRecord
    attrCatBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}
int BlockAccess::insert(int relId, Attribute *record) {
    // Get the relation catalog entry from relation cache
    RelCatEntry relCatEntry;
    int ret = RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if (ret != SUCCESS) {
        return ret;
    }

    int blockNum = relCatEntry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1;

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR until the end of the list is reached
    */
    while (blockNum != -1) {
        // Create a RecBuffer object for blockNum (using Constructor 2)
        RecBuffer recBuffer(blockNum);

        // Get header of block(blockNum)
        HeadInfo head;
        recBuffer.getHeader(&head);

        // Get slot map of block(blockNum)
        unsigned char slotMap[numOfSlots];
        recBuffer.getSlotMap(slotMap);

        // Search for free slot in the block 'blockNum'
        for (int slot = 0; slot < numOfSlots; slot++) {
            if (slotMap[slot] == SLOT_UNOCCUPIED) {
                rec_id.block = blockNum;
                rec_id.slot = slot;
                break;
            }
        }

        // If a free slot was found, break out of loop
        if (rec_id.block != -1) {
            break;
        }

        // Otherwise, update block numbers for next iteration
        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    // If no free slot was found in existing record blocks
    if (rec_id.block == -1 && rec_id.slot == -1) {
        // If relation is RELCAT, do not allocate any more blocks
        if (relId == RELCAT_RELID) {
            return E_MAXRELATIONS;
        }

        // Allocate a new record block using RecBuffer Constructor 1
        RecBuffer newBlock;

        // Get the block number of the newly allocated block
        int newBlockNum = newBlock.getBlockNum();
        if (newBlockNum == E_DISKFULL) {
            return E_DISKFULL;
        }

        // Assign rec_id
        rec_id.block = newBlockNum;
        rec_id.slot = 0;

        /*
            Set the header of the new record block such that it links with
            existing record blocks of the relation
        */
        HeadInfo head;
        head.blockType = REC;
        head.pblock = -1;
        head.lblock = (prevBlockNum == -1) ? -1 : prevBlockNum;
        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = numOfSlots;
        head.numAttrs = numOfAttributes;

        newBlock.setHeader(&head);

        /*
            Set block's slot map with all slots marked as free
        */
        unsigned char slotMap[numOfSlots];
        for (int i = 0; i < numOfSlots; i++) {
            slotMap[i] = SLOT_UNOCCUPIED;
        }
        newBlock.setSlotMap(slotMap);

        if (prevBlockNum != -1) {
            // Update rblock field in header of previous block
            RecBuffer prevBlock(prevBlockNum);
            HeadInfo prevHeader;
            prevBlock.getHeader(&prevHeader);
            prevHeader.rblock = newBlockNum;
            prevBlock.setHeader(&prevHeader);
        } else {
            // Update first block field in relation catalog entry
            relCatEntry.firstBlk = newBlockNum;
        }

        // Update last block field in relation catalog entry
        relCatEntry.lastBlk = newBlockNum;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // Create a RecBuffer object for rec_id.block
    RecBuffer targetBlock(rec_id.block);

    // Insert record into rec_id's slot
    targetBlock.setRecord(record, rec_id.slot);

    // Update slot map by marking entry of inserted slot as occupied
    unsigned char slotMap[numOfSlots];
    targetBlock.getSlotMap(slotMap);
    slotMap[rec_id.slot] = SLOT_OCCUPIED;
    targetBlock.setSlotMap(slotMap);

    // Increment numEntries field in block header
    HeadInfo head;
    targetBlock.getHeader(&head);
    head.numEntries++;
    targetBlock.setHeader(&head);

    // Increment number of records in relation catalog entry
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}
//------------------------------stage 7------------------------------------
int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
  if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
    return E_NOTPERMITTED;
  }
  //finds the rwlation in relcat using linearSearch
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  Attribute relNameAttr;
  strcpy(relNameAttr.sVal, relName);

  char relcatAttrName[ATTR_SIZE];
  strcpy(relcatAttrName, RELCAT_ATTR_RELNAME);

  RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, relNameAttr, EQ);
  if (relCatRecId.block == -1 && relCatRecId.slot == -1) {
    return E_RELNOTEXIST;
  }

  Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
  RecBuffer relCatBuffer(relCatRecId.block);
  int ret = relCatBuffer.getRecord(relCatEntryRecord, relCatRecId.slot);
  if (ret != SUCCESS) return ret;

  int currentBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;

  /*** release all record blocks of the relation ***/
  while (currentBlock != -1) {
    RecBuffer recBlock(currentBlock);

    HeadInfo head;
    ret = recBlock.getHeader(&head);
    if (ret != SUCCESS) return ret;

    int nextBlock = head.rblock;   // read before the block is released
    recBlock.releaseBlock();

    currentBlock = nextBlock;
  }

  /*** Delete the attribute catalog entries of the relation ***/
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  char attrcatAttrName[ATTR_SIZE];
  strcpy(attrcatAttrName, ATTRCAT_ATTR_RELNAME);

  int numberOfAttributesDeleted = 0;

  while (true) {
    RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, relNameAttr, EQ);
    if (attrCatRecId.block == -1 && attrCatRecId.slot == -1) {
      break;
    }

    numberOfAttributesDeleted++;

    RecBuffer attrCatBlock(attrCatRecId.block);

    HeadInfo attrCatHeader;
    ret = attrCatBlock.getHeader(&attrCatHeader);
    if (ret != SUCCESS) return ret;

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    ret = attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);
    if (ret != SUCCESS) return ret;

    int rootBlock = attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

    // mark the slot as unoccupied
    unsigned char slotMap[SLOTMAP_SIZE_RELCAT_ATTRCAT];
    ret = attrCatBlock.getSlotMap(slotMap);
    if (ret != SUCCESS) return ret;
    slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
    ret = attrCatBlock.setSlotMap(slotMap);
    if (ret != SUCCESS) return ret;

    attrCatHeader.numEntries--;
    ret = attrCatBlock.setHeader(&attrCatHeader);
    if (ret != SUCCESS) return ret;

    // block is now empty: unlink it from the linked list, then release it
    if (attrCatHeader.numEntries == 0) {
      RecBuffer leftBlock(attrCatHeader.lblock);
      HeadInfo leftHeader;
      ret = leftBlock.getHeader(&leftHeader);
      if (ret != SUCCESS) return ret;
      leftHeader.rblock = attrCatHeader.rblock;
      ret = leftBlock.setHeader(&leftHeader);
      if (ret != SUCCESS) return ret;

      if (attrCatHeader.rblock != -1) {
        RecBuffer rightBlock(attrCatHeader.rblock);
        HeadInfo rightHeader;
        ret = rightBlock.getHeader(&rightHeader);
        if (ret != SUCCESS) return ret;
        rightHeader.lblock = attrCatHeader.lblock;
        ret = rightBlock.setHeader(&rightHeader);
        if (ret != SUCCESS) return ret;
      } else {
        // released block was the last block of the attribute catalog
        RelCatEntry attrCatRelEntry;
        RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
        attrCatRelEntry.lastBlk = attrCatHeader.lblock;
        RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
      }

      attrCatBlock.releaseBlock();
    }

    // index cleanup, to be added in the indexing stage
    if (rootBlock != -1) {
      // BPlusTree::bPlusDestroy(rootBlock);
    }
  }

  /*** delete the relation's entry from the relation catalog ***/
  HeadInfo relCatHeader;
  ret = relCatBuffer.getHeader(&relCatHeader);
  if (ret != SUCCESS) return ret;
  relCatHeader.numEntries--;
  ret = relCatBuffer.setHeader(&relCatHeader);
  if (ret != SUCCESS) return ret;

  unsigned char relSlotMap[SLOTMAP_SIZE_RELCAT_ATTRCAT];
  ret = relCatBuffer.getSlotMap(relSlotMap);
  if (ret != SUCCESS) return ret;
  relSlotMap[relCatRecId.slot] = SLOT_UNOCCUPIED;
  ret = relCatBuffer.setSlotMap(relSlotMap);
  if (ret != SUCCESS) return ret;

  /*** update the relation cache entries of the catalogs ***/
  RelCatEntry relCatEntry;

  RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
  relCatEntry.numRecs--;
  RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

  RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
  relCatEntry.numRecs -= numberOfAttributesDeleted;
  RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);

  return SUCCESS;
}
//----------------------------------stage 9-------------------------------------------
int BlockAccess::project(int relId, Attribute *record) {
    // previous search index of this relation
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1) {
        // new project operation: start from the first record block
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    } else {
        // an operation is already in progress: continue after the previous record
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    // find the next occupied slot
    while (block != -1) {
        RecBuffer recBuffer(block);

        HeadInfo head;
        int ret = recBuffer.getHeader(&head);
        if (ret != SUCCESS) return ret;

        unsigned char slotMap[head.numSlots];
        ret = recBuffer.getSlotMap(slotMap);
        if (ret != SUCCESS) return ret;

        if (slot >= head.numSlots) {
            // no more slots in this block: move to the right block
            block = head.rblock;
            slot = 0;
        } else if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
        } else {
            // next occupied slot found
            break;
        }
    }

    if (block == -1) {
        // all records exhausted
        return E_NOTFOUND;
    }

    RecId nextRecId{block, slot};
    RelCacheTable::setSearchIndex(relId, &nextRecId);

    // copy the record into the caller's buffer
    RecBuffer recBuffer(nextRecId.block);
    int ret = recBuffer.getRecord(record, nextRecId.slot);
    if (ret != SUCCESS) return ret;

    return SUCCESS;
}