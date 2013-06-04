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

class CmpXchgVisitor : public InstVisitor<CmpXchgVisitor> {
    public:
        // Visitor function
        void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I);

        // Data accessor functions
        unsigned getSize() const;
        // Returns NULL if index out-of-bounds
        AtomicCmpXchgInst *getInst(unsigned index) const;

    private:
        // Vector of all the found store instructions
        std::vector<AtomicCmpXchgInst *> cmpXchgInsts;
};
