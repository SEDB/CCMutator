/*
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author Markus Kusano
 * \date 2013-06-02
 */
#include "LoadVisitor.h"

// Enable debugging output
//#define MUT_DEBUG

#ifdef MUT_DEBUG
#include "llvm/Support/raw_ostream.h"
#endif


LoadVisitor::LoadVisitor() {
    onlyAtomic = false;
}

void LoadVisitor::setOnlyAtomic(bool val) {
    onlyAtomic = val;
}

void LoadVisitor::visitLoadInst(LoadInst &I) {
#ifdef MUT_DEBUG
    errs() << "[DEBUG] Found Fence Inst\n";
#endif

    if (onlyAtomic) {
        if (I.isAtomic()) {
            loadInsts.push_back(&I);
        }
    }
    else
        loadInsts.push_back(&I);
}

unsigned LoadVisitor::getSize() const {
    return loadInsts.size();
}

LoadInst *LoadVisitor::getInst(unsigned index) const {
    if (index < getSize()) {
        return loadInsts[index];
    }
    else {
        return NULL;
    }
}
