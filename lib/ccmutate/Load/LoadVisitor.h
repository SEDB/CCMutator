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

class LoadVisitor : public InstVisitor<LoadVisitor> {
    public:
        LoadVisitor();
        // Visitor function
        void visitLoadInst(LoadInst &I);

        // Data accessor functions
        unsigned getSize() const;
        // Returns NULL if index out-of-bounds
        LoadInst *getInst(unsigned index) const;

        // Set the value of onlyAtomic. If true, this will make the visitor
        // only enumerate atomic loads
        void setOnlyAtomic(bool val);

    private:
        bool onlyAtomic;
        // Vector of all the found fence instructions
        std::vector<LoadInst *> loadInsts;
};
