#include "BPlusTree.h"

#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // search index of this attribute: where the previous hit was
    IndexId searchIndex;
    int ret = AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);
    if (ret != SUCCESS) {
        return RecId{-1, -1};
    }

    AttrCatEntry attrCatEntry;
    ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS) {
        return RecId{-1, -1};
    }

    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1) {
        // first search: start from the first entry of the root
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1) {
            // this attribute has no B+ tree
            return RecId{-1, -1};
        }
    } else {
        // resume from the entry after the previous hit (always a leaf block)
        block = searchIndex.block;
        index = searchIndex.index + 1;

        IndLeaf leaf(block);

        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries) {
            // this leaf is exhausted: continue from the start of the next leaf
            block = leafHead.rblock;
            index = 0;

            if (block == -1) {
                // end of the linked list: search is done
                return RecId{-1, -1};
            }
        }
    }

    /****** walk down through the internal nodes to a leaf ******/

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL) {
        IndInternal internalBlk(block);

        HeadInfo intHead;
        internalBlk.getHeader(&intHead);

        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE) {
            // always go left: NE needs the whole leaf list, and LT/LE hits
            // (if any) are in the leftmost leaf
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        } else {
            // EQ, GT, GE: go left of the first entry that is >= attrVal (> for GT)
            bool found = false;

            for (int entry = 0; entry < intHead.numEntries; entry++) {
                internalBlk.getEntry(&intEntry, entry);

                int cmpVal = compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType);

                if ((op == GT && cmpVal > 0) ||
                    ((op == EQ || op == GE) && cmpVal >= 0)) {
                    block = intEntry.lChild;
                    found = true;
                    break;
                }
            }

            if (!found) {
                // every key is smaller: go to the right child of the last entry
                internalBlk.getEntry(&intEntry, intHead.numEntries - 1);
                block = intEntry.rChild;
            }
        }
    }

    // `block` is now a leaf index block

    /****** scan the leaves for the first entry that satisfies the condition ******/

    while (block != -1) {
        IndLeaf leafBlk(block);

        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index < leafHead.numEntries) {
            leafBlk.getEntry(&leafEntry, index);

            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            if ((op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)) {
                // hit: remember where we are, return the record id
                searchIndex.block = block;
                searchIndex.index = index;
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);

                return RecId{leafEntry.block, leafEntry.slot};
            } else if ((op == EQ || op == LE || op == LT) && cmpVal > 0) {
                // leaf values are ascending, so no later entry can match
                return RecId{-1, -1};
            }

            ++index;
        }

        // only NE has to look at the whole leaf list
        if (op != NE) {
            break;
        }

        block = leafHead.rblock;
        index = 0;
    }

    // nothing satisfied the condition
    return RecId{-1, -1};
}
int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
        return E_NOTPERMITTED;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS) {
        return ret;
    }

    // an index already exists
    if (attrCatEntry.rootBlock != -1) {
        return SUCCESS;
    }

    /****** create the (empty) root: a leaf block ******/
    IndLeaf rootBlockBuf;
    int rootBlock = rootBlockBuf.getBlockNum();

    if (rootBlock == E_DISKFULL) {
        return E_DISKFULL;
    }

    // record the new root in the attribute cache (bPlusInsert reads it from there)
    attrCatEntry.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int block = relCatEntry.firstBlk;

    /****** insert every record of the relation into the tree ******/
    while (block != -1) {
        RecBuffer blockBuffer(block);

        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        blockBuffer.getSlotMap(slotMap);

        for (int slot = 0; slot < relCatEntry.numSlotsPerBlk; slot++) {
            if (slotMap[slot] == SLOT_OCCUPIED) {
                Attribute record[relCatEntry.numAttrs];
                blockBuffer.getRecord(record, slot);

                RecId recId{block, slot};

                // on E_DISKFULL, bPlusInsert has already destroyed the tree
                int retVal = bPlusInsert(relId, attrName, record[attrCatEntry.offset], recId);
                if (retVal == E_DISKFULL) {
                    return E_DISKFULL;
                }
            }
        }

        HeadInfo header;
        blockBuffer.getHeader(&header);
        block = header.rblock;
    }

    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum) {
    if (rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF) {
        IndLeaf leaf(rootBlockNum);
        leaf.releaseBlock();
        return SUCCESS;

    } else if (type == IND_INTERNAL) {
        IndInternal internalBlk(rootBlockNum);

        HeadInfo header;
        internalBlk.getHeader(&header);

        InternalEntry entry;

        if (header.numEntries > 0) {
            // the leftmost child, then the right child of every entry
            // (each right child is the next entry's left child, so nothing is freed twice)
            internalBlk.getEntry(&entry, 0);
            bPlusDestroy(entry.lChild);

            for (int i = 0; i < header.numEntries; i++) {
                internalBlk.getEntry(&entry, i);
                bPlusDestroy(entry.rChild);
            }
        }

        internalBlk.releaseBlock();
        return SUCCESS;

    } else {
        return E_INVALIDBLOCK;
    }
}
int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId) {
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS) {
        return ret;
    }

    int rootBlock = attrCatEntry.rootBlock;
    if (rootBlock == -1) {
        return E_NOINDEX;
    }

    int leafBlkNum = findLeafToInsert(rootBlock, attrVal, attrCatEntry.attrType);

    Index indexEntry;
    indexEntry.attrVal = attrVal;
    indexEntry.block = recId.block;
    indexEntry.slot = recId.slot;

    if (insertIntoLeaf(relId, attrName, leafBlkNum, indexEntry) == E_DISKFULL) {
        // out of space: throw the whole tree away
        bPlusDestroy(rootBlock);

        attrCatEntry.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType) {
    int blockNum = rootBlock;

    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF) {
        IndInternal internalBlk(blockNum);

        HeadInfo header;
        internalBlk.getHeader(&header);

        InternalEntry entry;
        bool found = false;

        // first entry whose key is >= the value being inserted
        for (int i = 0; i < header.numEntries; i++) {
            internalBlk.getEntry(&entry, i);

            if (compareAttrs(entry.attrVal, attrVal, attrType) >= 0) {
                blockNum = entry.lChild;
                found = true;
                break;
            }
        }

        if (!found) {
            // every key is smaller: go to the rightmost child
            internalBlk.getEntry(&entry, header.numEntries - 1);
            blockNum = entry.rChild;
        }
    }

    return blockNum;
}
int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry) {
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    IndLeaf leafBlk(blockNum);

    HeadInfo blockHeader;
    leafBlk.getHeader(&blockHeader);

    // existing entries plus the new one, in ascending order
    Index indices[blockHeader.numEntries + 1];

    bool inserted = false;
    int k = 0;
    for (int i = 0; i < blockHeader.numEntries; i++) {
        Index cur;
        leafBlk.getEntry(&cur, i);

        if (!inserted && compareAttrs(cur.attrVal, indexEntry.attrVal, attrCatEntry.attrType) > 0) {
            indices[k++] = indexEntry;
            inserted = true;
        }
        indices[k++] = cur;
    }
    if (!inserted) {
        indices[k++] = indexEntry;
    }

    if (blockHeader.numEntries != MAX_KEYS_LEAF) {
        // there is room: write everything back
        blockHeader.numEntries++;
        leafBlk.setHeader(&blockHeader);

        for (int i = 0; i < blockHeader.numEntries; i++) {
            leafBlk.setEntry(&indices[i], i);
        }

        return SUCCESS;
    }

    // leaf is full: split it
    int newRightBlk = splitLeaf(blockNum, indices);
    if (newRightBlk == E_DISKFULL) {
        return E_DISKFULL;
    }

    int ret;
    if (blockHeader.pblock != -1) {
        // not the root: push the middle key up into the parent
        InternalEntry intEntry;
        intEntry.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal;
        intEntry.lChild = blockNum;
        intEntry.rChild = newRightBlk;

        ret = insertIntoInternal(relId, attrName, blockHeader.pblock, intEntry);
    } else {
        // the root leaf was split: grow the tree by one level
        ret = createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal, blockNum, newRightBlk);
    }

    if (ret == E_DISKFULL) {
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]) {
    IndLeaf rightBlk;                 // new block
    IndLeaf leftBlk(leafBlockNum);    // existing block

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leftBlk.getBlockNum();

    if (rightBlkNum == E_DISKFULL) {
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    rightBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlkHeader.lblock = leftBlkNum;
    rightBlkHeader.rblock = leftBlkHeader.rblock;
    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    leftBlkHeader.rblock = rightBlkNum;
    leftBlk.setHeader(&leftBlkHeader);

    // first half to the left block, second half to the right block
    for (int i = 0; i < (MAX_KEYS_LEAF + 1) / 2; i++) {
        leftBlk.setEntry(&indices[i], i);
        rightBlk.setEntry(&indices[i + (MAX_KEYS_LEAF + 1) / 2], i);
    }

    return rightBlkNum;
}
int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry) {
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    IndInternal intBlk(intBlockNum);

    HeadInfo blockHeader;
    intBlk.getHeader(&blockHeader);

    // existing entries plus the new one, in ascending order
    InternalEntry internalEntries[blockHeader.numEntries + 1];

    bool inserted = false;
    int insertedAt = -1;
    int k = 0;
    for (int i = 0; i < blockHeader.numEntries; i++) {
        InternalEntry cur;
        intBlk.getEntry(&cur, i);

        if (!inserted && compareAttrs(cur.attrVal, intEntry.attrVal, attrCatEntry.attrType) > 0) {
            insertedAt = k;
            internalEntries[k++] = intEntry;
            inserted = true;
        }
        internalEntries[k++] = cur;
    }
    if (!inserted) {
        insertedAt = k;
        internalEntries[k++] = intEntry;
    }

    // the entry right after the new one must now start at the new entry's right child
    if (insertedAt + 1 < k) {
        internalEntries[insertedAt + 1].lChild = intEntry.rChild;
    }

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL) {
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);

        for (int i = 0; i < blockHeader.numEntries; i++) {
            intBlk.setEntry(&internalEntries[i], i);
        }

        return SUCCESS;
    }

    // block is full: split it
    int newRightBlk = splitInternal(intBlockNum, internalEntries);

    if (newRightBlk == E_DISKFULL) {
        // the subtree below the new entry is not attached to the tree yet: free it
        bPlusDestroy(intEntry.rChild);
        return E_DISKFULL;
    }

    int ret;
    if (blockHeader.pblock != -1) {
        InternalEntry parentEntry;
        parentEntry.attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;
        parentEntry.lChild = intBlockNum;
        parentEntry.rChild = newRightBlk;

        ret = insertIntoInternal(relId, attrName, blockHeader.pblock, parentEntry);
    } else {
        ret = createNewRoot(relId, attrName, internalEntries[MIDDLE_INDEX_INTERNAL].attrVal, intBlockNum, newRightBlk);
    }

    if (ret == E_DISKFULL) {
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]) {
    IndInternal rightBlk;                   // new block
    IndInternal leftBlk(intBlockNum);       // existing block

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leftBlk.getBlockNum();

    if (rightBlkNum == E_DISKFULL) {
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    rightBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    leftBlk.setHeader(&leftBlkHeader);

    // entries 0..49 stay left, entry 50 moves up to the parent, entries 51..100 go right
    for (int i = 0; i < MAX_KEYS_INTERNAL / 2; i++) {
        leftBlk.setEntry(&internalEntries[i], i);
        rightBlk.setEntry(&internalEntries[i + MAX_KEYS_INTERNAL / 2 + 1], i);
    }

    // the children of the right block now have a new parent. Those children are
    // the right children of entries 50..100 (entry 50's right child is entry 51's left child)
    for (int i = MAX_KEYS_INTERNAL / 2; i <= MAX_KEYS_INTERNAL; i++) {
        BlockBuffer childBlock(internalEntries[i].rChild);

        HeadInfo childHeader;
        childBlock.getHeader(&childHeader);
        childHeader.pblock = rightBlkNum;
        childBlock.setHeader(&childHeader);
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild) {
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    IndInternal newRootBlk;
    int newRootBlkNum = newRootBlk.getBlockNum();

    if (newRootBlkNum == E_DISKFULL) {
        // the right subtree is not attached to the tree yet: free it
        bPlusDestroy(rChild);
        return E_DISKFULL;
    }

    HeadInfo header;
    newRootBlk.getHeader(&header);
    header.numEntries = 1;
    newRootBlk.setHeader(&header);

    InternalEntry entry;
    entry.lChild = lChild;
    entry.attrVal = attrVal;
    entry.rChild = rChild;
    newRootBlk.setEntry(&entry, 0);

    // both children now have the new root as their parent
    BlockBuffer leftChild(lChild);
    BlockBuffer rightChild(rChild);

    HeadInfo childHeader;

    leftChild.getHeader(&childHeader);
    childHeader.pblock = newRootBlkNum;
    leftChild.setHeader(&childHeader);

    rightChild.getHeader(&childHeader);
    childHeader.pblock = newRootBlkNum;
    rightChild.setHeader(&childHeader);

    // point the attribute cache at the new root
    attrCatEntry.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

    return SUCCESS;
}
