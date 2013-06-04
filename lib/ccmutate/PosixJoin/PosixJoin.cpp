/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file PosixJoin.cpp
 * Author: Markus Kusano
 * Date: 2013-01-11
 *
 * Pass to mutate a CallInst to pthread_join. See README.md for more information.
 */

#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"
#include "llvm/IRBuilder.h"

#include "llvm/Support/CommandLine.h"

//#include "FindPosixJoinVisitor.h"
#include "../Tools/EnumerateCallInst.h"

#define MUT_DEBUG

using namespace llvm;

// Command line flags
static cl::opt<bool> 
    verbose("v", 
	cl::desc("Enable verbose output. "
	    "Displays filename and location of occurances"),
	cl::init(false));

// Specifies if we are in remove mode
static cl::opt<bool> rmMode("rmmode", 
	cl::desc("remove occurances of pthread_join"),
	cl::init(false));

// Specifies if we are in replace join with sleep mode
static cl::opt<bool> repMode("repmode", 
	cl::desc("replace occurance of pthread_join with sleep"),
	cl::init(false));

// Command line option that specifies which occurance of pthread_join should be
// removed. Use the analysis FindPosixJoin to get the enumeration. Specify a
// comma seperated list of 0 indexed values (e.g. -pos=1,3,4) which will
// remove occurances 1, 3 and 4.
static cl::list<unsigned> PosToRm("pos", 
	cl::desc("occurances to remove or replace with sleep"),
	cl::value_desc("comma seperated list of occurances to alter"),
	cl::CommaSeparated);

static cl::opt<unsigned> SleepValue("sleepval",
	cl::desc("value to be passed to call to sleep, default is 1"),
	cl::value_desc("positive integer"),
	cl::init(1));

namespace {
  struct FindPosixJoin: public ModulePass {
    static char ID;
    FindPosixJoin() : ModulePass(ID) { numCalls = 0; }

    /**
     * Visitor to find CallInst to pthread_join
     */
    EnumerateCallInst pjv;

    /**
     * Number of calls to pthread_join found
     */
    unsigned numCalls;

    /**
     * Uses a FindPosixJoinVisitor to find occurances of CallInst to
     * pthread_join
     */
    virtual bool runOnModule(Module &M) {
	errs() << "FindPosixJoin: \n";
	pjv.addFuncNameToSearch("pthread_join");
	pjv.visit(M);

#ifdef MUT_DEBUG
	DEBUG(errs() << "DEBUG: Found " << pjv.callInsts.size()
		     << " instances of calls to pthread_join() \n");
#endif

	numCalls = pjv.callInsts.size();

	bool modified; /**< Indicates if the program was modified */
	modified = false;

	// Check if both rm and replace were specified.
	if (rmMode && repMode) {
	    errs() << "Error: both remove mode and replace mode were specified\n";
	    exit(EXIT_FAILURE);
	}

	if (rmMode) {
#ifdef MUT_DEBUG
	    DEBUG(errs() << "DEBUG: in remove mode\n");
#endif

	    // Check if no position to remove were specified
	    if (PosToRm.size() == 0) {
		errs() << "Warning: In remove mode but no positions specified\n";
	    }

	    for (unsigned i = 0; i < PosToRm.size(); i++) {
		// pthread_join returns an int, so replace occurrence with a
		// zero of that size if it still has uses
		int ret = pjv.removeFromParentRepZero(PosToRm[i], sizeof(int), true);

#ifdef MUT_DEBUG
		DEBUG(errs() << "DEBUG: remove from parent returned: " << ret << '\n');
#endif


		if (ret == -1) {
		    errs() << "Warning: position " << PosToRm[i] << " is out of "
			   << "bounds of found instructions, ignoring\n";
		}
		else if (ret == -2) {
		    errs() << "Warning: position " << PosToRm[i] << " has been "
			   << "removed already. Skipping\n";
		}
		else {
		    modified = true;
		}
	    }
	}
	else if (repMode) {
#ifdef MUT_DEBUG
	    DEBUG(errs() << "DEBUG: in replace mode\n");
#endif
	    for (unsigned i = 0; i < PosToRm.size(); i++) {
		int ret = pjv.isValidIndex(PosToRm[i]);
		if (ret) {
		    if (ret == -1) {
			errs() << "Warning: attempting to modify instruction out-of-bounds "
				  " of found instructions, skipping\n";
		    }
		    else if (ret == -2) {
			errs() << "Warning: attempting to modify the same instruction twice, "
				  "skipping\n";
		    }
		    continue; // attempt to mutate the next instruction
		}
		    
#ifdef MUT_DEBUG
		DEBUG(errs() << "DEBUG: replacing  occurence: " << PosToRm[i]
			     << '\n');
#endif

		// Create a call to sleep() and replace the current call to
		// pthread_join() with it using the EnumerateCallInst
		// object to keep track of instructions that have already
		// been modified.
		ConstantInt *sleepArg =
		    ConstantInt::get(IntegerType::get(getGlobalContext(), 
				sizeof(unsigned) * 8), SleepValue, true);
		ArrayRef<Value *> args(sleepArg);
		IRBuilder<> builder(pjv.callInsts[PosToRm[i]]);
		Constant *c = M.getOrInsertFunction("sleep", 
			IntegerType::get(getGlobalContext(), sizeof(unsigned) * 8),
			IntegerType::get(getGlobalContext(), sizeof(unsigned) * 8), NULL);
		CallInst *sleepCall = CallInst::Create(c, args, "sleep_mut");

		ret = pjv.replaceInstWithInst(PosToRm[i], sleepCall);
		if (ret == -1) {
		    errs() << "Warning: attempting to modify instruction out-of-bounds "
			      " of found instructions, skipping\n";
		}
		else if (ret == -2) {
		    errs() << "Warning: attempting to modify the same instruction twice, "
			      "skipping\n";
		}
		else {
		    modified = true;
		}
	    }
	}

	// Use print method (-print) to display the results
	//errs() << numCalls << '\n';

	return modified;
    }

    /**
     * TODO: We only sometimes modify the program. Incase we run in a string of
     * passes controlled by pass manager then maybe we should let the manager
     * know when we do/do not invalidate the analyses. But, I can't see how
     * this can be run in pass manager since it requires command line arguments
     */

    /**
     * Prints the results of the analysis. In non verbose mode this simply
     * prints the number of calls to pthread_join found. Verbose mode prints
     * out the filename and source line number of each occurance found
     * seperated by a tab and each on a new line:
     *	    <filename>\t<source line numer>
     */
    virtual void print(llvm::raw_ostream &O, const Module *M) const {
	if (!verbose) {
	    errs() << numCalls << '\n';
	}
	else {
#ifdef MUT_DEBUG
	    DEBUG(errs() << "DEBUG: Verbose mode activated" << '\n');
#endif
	    // Iterate over each pthread_join call instruction and print out
	    // it's information.
	    MDNode *metaNode;
	    for (unsigned i = 0; i < pjv.callInsts.size(); i++) {
		metaNode = pjv.callInsts[i]->getMetadata("dbg");
		if (metaNode) {
		    DILocation Loc(metaNode);
		    StringRef File = Loc.getFilename();
		    unsigned Line = Loc.getLineNumber();
		    errs() << File << '\t' << Line << '\n';
		}
		else {
		    errs() << "Warning: No meta data found for instruction "
			   << i << '\n';
		}
	    }
	}
    }

    
  };
}

char FindPosixJoin::ID = 0;
static RegisterPass<FindPosixJoin> X("PosixJoin", "Find Occurences of pthread_join", false, false);
