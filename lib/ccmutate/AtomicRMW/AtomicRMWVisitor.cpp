/*
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author: Markus Kusano
 * \date: 2013-06-02
 */

#include "AtomicRMWVisitor.h"

// Enable debugging output
//#define MUT_DEBUG

#ifdef MUT_DEBUG
#include "llvm/Support/raw_ostream.h"
#endif


void AtomicRMWVisitor::visitAtomicRMWInst(AtomicRMWInst &I) {
#ifdef MUT_DEBUG
    errs() << "[DEBUG] Found atomicrmw Inst\n";
#endif

    atomicRMWInsts.push_back(&I);
}

unsigned AtomicRMWVisitor::getSize() const {
    return atomicRMWInsts.size();
}

AtomicRMWInst *AtomicRMWVisitor::getInst(unsigned index) const {
    if (index < getSize()) {
        return atomicRMWInsts[index];
    }
    else {
        return NULL;
    }
}
