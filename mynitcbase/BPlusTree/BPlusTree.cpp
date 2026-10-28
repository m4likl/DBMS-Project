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