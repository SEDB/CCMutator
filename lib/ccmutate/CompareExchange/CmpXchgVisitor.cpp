/*
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author: Markus Kusano
 * \date: 2013-06-02
 */

#include "CmpXchgVisitor.h"

// Enable debugging output
//#define MUT_DEBUG

#ifdef MUT_DEBUG
#include "llvm/Support/raw_ostream.h"
#endif


void CmpXchgVisitor::visitAtomicCmpXchgInst(AtomicCmpXchgInst &I) {
#ifdef MUT_DEBUG
    errs() << "[DEBUG] Found cmpXchng Inst\n";
#endif

    cmpXchgInsts.push_back(&I);
}

unsigned CmpXchgVisitor::getSize() const {
    return cmpXchgInsts.size();
}

AtomicCmpXchgInst *CmpXchgVisitor::getInst(unsigned index) const {
    if (index < getSize()) {
        return cmpXchgInsts[index];
    }
    else {
        return NULL;
    }
}
