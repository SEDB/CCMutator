/*
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author: Markus Kusano
 * \date: 2013-06-02
 */

#include "llvm/Support/InstVisitor.h"
#include <vector>

using namespace llvm;

class AtomicRMWVisitor : public InstVisitor<AtomicRMWVisitor> {
    public:
        // Visitor function
        void visitAtomicRMWInst(AtomicRMWInst &I);

        // Data accessor functions
        unsigned getSize() const;
        // Returns NULL if index out-of-bounds
        AtomicRMWInst *getInst(unsigned index) const;

    private:
        // Vector of all the found store instructions
        std::vector<AtomicRMWInst *> atomicRMWInsts;
};
