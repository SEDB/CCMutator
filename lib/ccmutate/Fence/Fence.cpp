/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \author: Markus Kusano
 * \date: 2013-06-02
 * \file Fence.cpp
 *
 * Mutation pass for Fence instructions
 */
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "FenceVisitor.h"
#include "../Tools/FileInfo.h"

using namespace llvm;

// Enable Debugging Output
//#define MUT_DEBUG

static cl::opt<bool> rmMode("rm", 
    cl::desc("remove occurances of fence instruction"),
    cl::init(false));

static cl::opt<bool> modMode("mod", 
    cl::desc("modify atomic ordering, specify type with -order"),
    cl::init(false));

static cl::opt<bool> scopeMode("scope", 
    cl::desc("toggle the scope of the fence from singlethreaded to multithreaded"),
    cl::init(false));

static cl::list<unsigned> orderings("order",
    cl::desc("ordering values to use with mod, each corresponds to a position in -pos"),
    cl::value_desc("comma separated, 0 == acquire, 1 == release, 2 == acq_rel, 3 == seq_cst"),
    cl::CommaSeparated);

static cl::list<unsigned> positions("pos", 
    cl::desc("occurances to remove or modify"),
    cl::value_desc("comma separated list of unsigned ints"),
    cl::CommaSeparated);


static cl::opt<bool> verbose("verbose", 
        cl::desc("enable verbose output\n"),
        cl::init(false));

namespace {
struct Fence : public ModulePass {
    static char ID;
    FenceVisitor fenceInsts;
    Fence() : ModulePass(ID) { }


    virtual bool runOnModule(Module &M) {
        bool modified; 

        // Initialize
        modified = false;

        checkCommandLineArgs();

        fenceInsts.visit(M);

        if (rmMode) {
#ifdef MUT_DEBUG
            errs() << "[DEBUG] In rmMode\n";
#endif
            removeInstructions();
            modified = true;
        }
        else if (modMode) {
            modifyInstructions();
            modified = true;
        }
        else { // scopeMode
            toggleScope();
            modified = true;
        }

#ifdef MUT_DEBUG
        errs() << "[DEBUG] exiting runOnModule\n";
#endif
        return modified;

    }

    void toggleScope() {
        for (unsigned i = 0; i < positions.size(); i++) {
            unsigned curIndex;
            FenceInst *curInst;

            curIndex = positions[i];
            if (indexOutOfBounds(curIndex)) {
                errs() << "Warning: index " << curIndex << " is out of bounds, skipping\n";
                continue;
            }

            curInst = fenceInsts.getInst(curIndex); // non NULL since position is in bounds

            if (curInst->getSynchScope() == SingleThread) {
                curInst->setSynchScope(CrossThread);
            }
            else {
                curInst->setSynchScope(SingleThread);
            }
        }
    }

    void removeInstructions() {
        for (unsigned i = 0; i < positions.size(); i++) {
            unsigned curIndex;
            FenceInst *curInst;

            curIndex = positions[i];
            if (indexOutOfBounds(curIndex)) {
                errs() << "Warning: index " << curIndex << " is out of bounds, skipping\n";
                continue;
            }

            curInst = fenceInsts.getInst(curIndex); // non NULL since position is in bounds
            curInst->eraseFromParent();
        }
    }

    void modifyInstructions() {
        for (unsigned i = 0; i < positions.size(); i++) {
            unsigned curIndex;
            AtomicOrdering aorder;
            FenceInst *curInst;

            curIndex = positions[i];
            if (indexOutOfBounds(curIndex)) {
                errs() << "Warning: index " << curIndex << " is out of bounds, skipping\n";
                continue;
            }

            curInst = fenceInsts.getInst(curIndex); // non NULL since position is in bounds

            aorder = getOrdering(curIndex);

            curInst->setOrdering(aorder);
        }
    }

    // Returns the corresponding value in the orderings list if the index is in
    // bounds. Otherwise, returns the last value in the list.
    AtomicOrdering getOrdering(unsigned index) {
        if (index < orderings.size()) {
            return unsignedToOrdering(orderings[index]);
        }
        else {
            return unsignedToOrdering(orderings.back());
        }
    }

    // Returns the corresponding AtomicOrdering from an unsigned. Valid values are:
    // 0 -> acquire
    // 1 -> release
    // 2 -> acq_release
    // 3 -> seq_cst
    // Returns acquire if out-of-bounds
    AtomicOrdering unsignedToOrdering(unsigned val) {
        AtomicOrdering ret;
        switch (val) {
            case 0:
                ret = Acquire;
                break;
            case 1:
                ret = Release;
                break;
            case 2:
                ret = AcquireRelease;
                break;
            case 3:
                ret = SequentiallyConsistent;
                break;
            default:
                ret = Acquire;
        }

        return ret;
    }



    bool indexOutOfBounds(unsigned index) {
        if (fenceInsts.getSize() < index) {
            return true;
        }
        return false;
    }

    virtual void print(llvm::raw_ostream &O, const Module *M) const {
        if (!verbose)
            errs() << fenceInsts.getSize() << '\n';
        else {
            for (unsigned i = 0; i < fenceInsts.getSize(); i++) {
                StringRef filename;
                unsigned linenum;

                filename = getDebugFilename(fenceInsts.getInst(i));
                linenum = getDebugLineNum(fenceInsts.getInst(i));
                errs() << i << '\t' << filename << ':' << linenum << "\n\t";
                errs() << *(fenceInsts.getInst(i)) << '\n';
            }
        }
    }

    // Exits if invalid command line values are found
    void checkCommandLineArgs() {
        if (rmMode && modMode) {
            errs() << "Error: -rm and -mod specified\n";
            exit(EXIT_FAILURE);
        }
        if (rmMode && scopeMode) {
            errs() << "Error: -rm and -scope specified\n";
            exit(EXIT_FAILURE);
        }
        if (modMode && scopeMode) {
            errs() << "Error: -mod and -scope specified\n";
            exit(EXIT_FAILURE);
        }
        if (rmMode && positions.size() == 0) {
            errs() << "Error: -rm but no positions specified with -pos\n";
            exit(EXIT_FAILURE);
        }
        if (modMode && positions.size() == 0) {
            errs() << "Error: -mod but no positions specified with -pos\n";
            exit(EXIT_FAILURE);
        }
        if (scopeMode && positions.size() == 0) {
            errs() << "Error: -scope but no positions specified with -pos\n";
            exit(EXIT_FAILURE);
        }
        if (modMode && orderings.size() == 0) {
            errs() << "Error -mod but no orderings specified with -order\n";
            exit(EXIT_FAILURE);
        }

        for (unsigned i = 0; i < orderings.size(); i++) {
            if (orderings[i] > 3) {
                errs() << "Error: atomic ordering value " << orderings[i] << " is too large (see -help)\n";
                exit(EXIT_FAILURE);
            }
        }
    }

}; // end struct fence 
} // end namespace (anon)


char Fence::ID = 0;
static RegisterPass<Fence> X("Fence", "Mutate Fence Instructions", false, false);
