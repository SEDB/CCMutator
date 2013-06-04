/*
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author Markus Kusano
 * \date 2013-06-02
 */
#include "llvm/Support/InstVisitor.h"
#include <vector>

using namespace llvm;

class FenceVisitor : public InstVisitor<FenceVisitor> {
    public:
        // Visitor function
        void visitFenceInst(FenceInst &I);

        // Data accessor functions
        unsigned getSize() const;
        // Returns NULL if index out-of-bounds
        FenceInst *getInst(unsigned index) const;

    private:
        // Vector of all the found fence instructions
        std::vector<FenceInst *> fenceInsts_m;
};
