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

class StoreVisitor : public InstVisitor<StoreVisitor> {
    public:
        StoreVisitor();
        // Visitor function
        void visitStoreInst(StoreInst &I);

        // Data accessor functions
        unsigned getSize() const;
        // Returns NULL if index out-of-bounds
        StoreInst *getInst(unsigned index) const;

        // Set the value of onlyAtomic. If true, this will make the visitor
        // only enumerate atomic loads
        void setOnlyAtomic(bool val);

    private:
        bool onlyAtomic;
        // Vector of all the found store instructions
        std::vector<StoreInst *> storeInsts;
};
