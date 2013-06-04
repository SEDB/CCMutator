/*
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author: Markus Kusano
 * \date: 2013-06-02
 */

#include "StoreVisitor.h"

// Enable debugging output
//#define MUT_DEBUG

#ifdef MUT_DEBUG
#include "llvm/Support/raw_ostream.h"
#endif


StoreVisitor::StoreVisitor() {
    onlyAtomic = false;
}

void StoreVisitor::setOnlyAtomic(bool val) {
    onlyAtomic = val;
}

void StoreVisitor::visitStoreInst(StoreInst &I) {
#ifdef MUT_DEBUG
    errs() << "[DEBUG] Found Fence Inst\n";
#endif

    if (onlyAtomic) {
        if (I.isAtomic()) {
            storeInsts.push_back(&I);
        }
    }
    else
        storeInsts.push_back(&I);
}

unsigned StoreVisitor::getSize() const {
    return storeInsts.size();
}

StoreInst *StoreVisitor::getInst(unsigned index) const {
    if (index < getSize()) {
        return storeInsts[index];
    }
    else {
        return NULL;
    }
}
