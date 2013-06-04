/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author: Markus Kusano
 * \date: 2013-06-02
 *
 * \file Store.cpp
 *
 * Mutation pass for Store instructions
 */

#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "CmpXchgVisitor.h"
#include "../Tools/FileInfo.h"

using namespace llvm;

// Enable Debugging Output
//#define MUT_DEBUG


static cl::list<unsigned> positions("pos", 
    cl::desc("occurances to remove or modify"),
    cl::value_desc("comma separated list of unsigned ints"),
    cl::CommaSeparated);


static cl::opt<bool> verbose("verbose", 
        cl::desc("enable verbose output\n"),
        cl::init(false));

// To be used in the future to support non-atomic to atomic load mutation
static cl::opt<bool> onlyAtomic("onlyatomic", 
        cl::desc("only enumerate and mutate atomic loads"),
        cl::Hidden,
        cl::init(true));

static cl::opt<bool> modMode("mod", 
        cl::desc("change atomic ordering of load instruction, use -order to specify ordering"),
        cl::init(false));

static cl::list<unsigned> orderings("order",
        cl::desc("atomic ordering values for each position found in -pos"),
        cl::value_desc("comma separated list of unsigned ints. 0 = monotonic, "
                       "1 = acquire, 2 = release, 3 = acquire release, 4 = sequentially consistent"),
        cl::CommaSeparated);

static cl::opt<bool> scope("scope", 
        cl::desc("change synchronization scope from single-threaded to multi-threaded and vice-versa"),
        cl::init(false));


namespace {
struct CmpXchg : public ModulePass {
    static char ID;
    CmpXchgVisitor cmpXchgInsts;
    CmpXchg() : ModulePass(ID) { }


    virtual bool runOnModule(Module &M) {
        bool modified; 

        // Initialize
        modified = false;

        checkCommandLineArgs();

        cmpXchgInsts.visit(M);

        if (modMode) {
            modifyInstructions();
            modified = true;
        }
        else { // scope
            toggleScope();
            modified = true;
        }

#ifdef MUT_DEBUG
        errs() << "[DEBUG] exiting runOnModule\n";
#endif
        return modified;

    }

    bool indexOutOfBounds(unsigned index) {
        if (cmpXchgInsts.getSize() < index) {
            return true;
        }
        return false;
    }

    void toggleScope() {
        for (unsigned i = 0; i < positions.size(); i++) {
            unsigned curIndex;
            AtomicCmpXchgInst *curInst;

            curIndex = positions[i];
            if (curIndex < cmpXchgInsts.getSize()) {
                curInst = cmpXchgInsts.getInst(curIndex);
                if (curInst->getSynchScope() == CrossThread)
                    curInst->setSynchScope(SingleThread);
                else // SingleThread
                    curInst->setSynchScope(CrossThread);
            }
            else {
                outOfBoundsWarning(curIndex);
            }
        }
    }

    void modifyInstructions() {
        for (unsigned i = 0; i < positions.size(); i++) {
            unsigned curIndex;
            AtomicCmpXchgInst *curInst;

            curIndex = positions[i];
            if (curIndex < cmpXchgInsts.getSize()) {
                AtomicOrdering aorder;
                curInst = cmpXchgInsts.getInst(curIndex); // non null since position in-bounds
                aorder = getOrdering(curIndex);
                curInst->setOrdering(aorder);
            }
            else {
                outOfBoundsWarning(curIndex);
            }
        }
    }

    // Returns the atomic ordering specified with -order of the given position.
    // If pos is out-of-bounds of -order list then it returns the last value in
    // -order
    AtomicOrdering getOrdering(unsigned index) {
        if (index < orderings.size()) {
            return unsignedToOrdering(orderings[index]);
        }
        else {
            return unsignedToOrdering(orderings.back());
        }
    }

    // Returns the corresponding AtomicOrdering from an unsigned. Valid values are:
    // 0 -> Monotonic
    // 1 -> Acquire
    // 2 -> Release
    // 3 -> AcquireRelease
    // 4 -> SequentiallyConsistent
    // Otherwise returns Unordered
    AtomicOrdering unsignedToOrdering(unsigned val) {
        AtomicOrdering ret;
        switch (val) {
            case 0:
                ret = Monotonic;
                break;
            case 1:
                ret = Acquire;
                break;
            case 2:
                ret = Release;
                break;
            case 3:
                ret = AcquireRelease;
                break;
            case 4:
                ret = SequentiallyConsistent;
                break;
            default:
                ret = Unordered;
        }
        return ret;
    }
    
    void outOfBoundsWarning(unsigned curIndex) {
        errs() << "Warning: position " << curIndex << " is out-of-bounds "
               << "of found instructions, skipping\n";
    }

    virtual void print(llvm::raw_ostream &O, const Module *M) const {
        if (!verbose) {
            errs() << cmpXchgInsts.getSize() << '\n';
        }
        else {
            for (unsigned i = 0; i < cmpXchgInsts.getSize(); i++) {
                StringRef filename;
                unsigned linenum;

                filename = getDebugFilename(cmpXchgInsts.getInst(i));
                linenum = getDebugLineNum(cmpXchgInsts.getInst(i));
                errs() << i << '\t' << filename << ':' << linenum << "\n\t";
                errs() << *(cmpXchgInsts.getInst(i)) << '\n';
            }
        }
    }

    // Exits if invalid command line values are found
    void checkCommandLineArgs() {
        if (modMode && positions.size() == 0) {
            errs() << "Error: -mod but no positions specified with -pos\n";
            exit(EXIT_FAILURE);
        }
        if (modMode && orderings.size() == 0) {
            errs() << "Error: -mod but no orderings specified with -order\n";
            exit(EXIT_FAILURE);
        }
        if (modMode && scope) {
            errs() << "Error: -mod and -scope can not both be specified\n";
            exit(EXIT_FAILURE);
        }
        if (scope && positions.size() == 0) {
            errs() << "Error: -scope but not positions specified with -pos\n";
            exit(EXIT_FAILURE);
        }

        // Ensure valid orderings
        for (unsigned i = 0; i < orderings.size(); i++) {
            if (orderings[i] > 4) {
                errs() << "Error: ordering value at index " << i << " is too large\n";
                exit(EXIT_FAILURE);
            }
        }
    }

}; // end struct fence 
} // end namespace (anon)


char CmpXchg::ID = 0;
static RegisterPass<CmpXchg> X("CmpXchg", "Mutate cmpxchg Instructions", false, false);
