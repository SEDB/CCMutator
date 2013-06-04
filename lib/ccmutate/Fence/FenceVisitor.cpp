/*
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author Markus Kusano
 * \date 2013-06-02
 */
#include "FenceVisitor.h"

// Enable debugging output
//#define MUT_DEBUG

#ifdef MUT_DEBUG
#include "llvm/Support/raw_ostream.h"
#endif


void FenceVisitor::visitFenceInst(FenceInst &I) {
#ifdef MUT_DEBUG
    errs() << "[DEBUG] Found Fence Inst\n";
#endif

    fenceInsts_m.push_back(&I);
}

unsigned FenceVisitor::getSize() const {
    return fenceInsts_m.size();
}

FenceInst* FenceVisitor::getInst(unsigned index) const {
    if (index < getSize()) {
        return fenceInsts_m[index];
    }
    else {
        return NULL;
    }
}
