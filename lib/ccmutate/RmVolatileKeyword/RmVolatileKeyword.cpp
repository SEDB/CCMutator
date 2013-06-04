/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.

 * \file RmVolatileKeyword.cpp
 * Author: Markus Kusano
 * Date: 2013-01-11
 *
 * Remove volatile keyword from some LLVM IR instructions
 */

#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"
#include "VolatileVisitor.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;


// Specifies verbose output
static cl::opt<bool> 
    verbose("v", 
	cl::desc("Enable verbose output."),
	cl::init(false));

// Specifies occurances to remove, e.g. -rmpos=1,2,3 which is equivalant to
// -rmpos=1 -rmpos=2 -rmpos=3.
static cl::list<unsigned> PosToRm("rmpos", 
	cl::desc("occurances to remove"),
	cl::value_desc("comma seperated list of occurances to remove"),
	cl::CommaSeparated, cl::ZeroOrMore);

/// Sets the passed instruction as non-volatile if it is a LoadInst,
/// StoreInst, AtomicCmpXchgInst, AtomicRMWInst, or llvm.{memcpy, memmove,
/// memset}
bool removeVolatile(Instruction *I);

namespace {
struct RmVolatileKeyword : public ModulePass {
    static char ID;

    VolatileVisitor volVis;

    /// Number of volatile instructions found
    unsigned numInsts;

    RmVolatileKeyword() : ModulePass(ID) { numInsts = 0; }

    virtual bool runOnModule(Module &M) {
	errs() << "RmVolatileKeyword\n";
	volVis.visit(M);
	DEBUG(errs() << "DEBUG: Found " << volVis.getVolaInstsSize()
		     << " instances of potentially volatile instructions\n");
	numInsts = volVis.getVolaInstsSize();

	bool modified; ///< Indicates if the program was modified
	modified = false;

	if (PosToRm.size() > 0) {
	    DEBUG(errs() << "DEBUG: In remove mode\n");
	    for (unsigned i = 0; i < PosToRm.size(); i++) {
		if (PosToRm[i] >= volVis.getVolaInstsSize()) {
		    errs() << "Warning: position to remove out-of-bounds of "
			   << "occurances of volatile instructions, "
			   << "skipping position: " << PosToRm[i] << '\n';
		}
		else {
		    DEBUG(errs() << "DEBUG: Attempting to remove volatile keyword from "
			    "instruction " << PosToRm[i] << '\n');
		    Instruction *I = volVis.getVolaInst(i);
		    if (I == NULL) {
			errs() << "Warning: position " << i << " is out-of-bounds of "
			       << "found volatile instructions, skipping\n";
		    }
		    else {
			modified = removeVolatile(I);
		    }
		}
	    }
	}

	return modified;
    }


    /**
     * TODO: We only sometimes modify the program. Incase we run in a string of
     * passes controlled by pass manager then maybe we should let the manager
     * know when we do/do not invalidate the analyses. But, I can't see how
     * this can be run in pass manager since it requires command line arguments
     */

    /// In non verbose mode, simply prints out the number of volatile /
    /// instructions found. In verbose mode, prints out filename and linenumber.
    virtual void print(llvm::raw_ostream &O, const Module *M) const {
	if (!verbose) {
	    errs() << numInsts << '\n';
	}
	else {
	    // Iterate over each found instruction and output file and line
	    // number information
	    for (unsigned i = 0; i < volVis.getVolaInstsSize(); i++) {
		Instruction *I = volVis.getVolaInst(i);
		if (MDNode *N = I->getMetadata("dbg")) {
		    DILocation Loc(N);
		    errs() << Loc.getFilename() << '\t' << Loc.getLineNumber() << '\n';
		}
		else {
		    errs() << "Warning: Instruction found with no debug metadata\n";
		}
	    }
	}
    }

}; // struct RmVolatileKeyword
} // namespace
bool removeVolatile(Instruction *I) {
    // All of the instructions checked except for the llvm intrinsic
    // instructions have a specific keyword attached to them. For the
    // intrinsic instructions, volatile is the 5th operand
    if (LoadInst *load = dyn_cast<LoadInst>(I)) {
	// the visitor function already filtered out non volatile
	// instructions
	DEBUG(errs() << "DEBUG: Load instruction set non-volatile\n");
	load->setVolatile(false);
    }
    else if (StoreInst *store = dyn_cast<StoreInst>(I)) {
	DEBUG(errs() << "DEBUG: Store instruction set non-volatile\n");
	store->setVolatile(false);
    }
    else if (AtomicCmpXchgInst *cmp = dyn_cast<AtomicCmpXchgInst>(I)) {
	DEBUG(errs() << "DEBUG: AtomicCmpXchngInst set non-volatile\n");
	cmp->setVolatile(false);
    }
    else if (AtomicRMWInst *rmw = dyn_cast<AtomicRMWInst>(I)) {
	DEBUG(errs() << "DEBUG: AtomicRMWINst set non-volatile\n");
	rmw->setVolatile(false);
    }
    else {
	// instruction does not match any of the above
	return false;
    }
    return true;
}



char RmVolatileKeyword::ID = 0;
static RegisterPass<RmVolatileKeyword> X("RmVolatileKeyword", "Find/Remove occurances of volatile instructions", false, false);
