/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file PosixCondSignal.cpp
 * Author: Markus Kusano
 * Date: 2013-01-11
 *
 */

#include "llvm/Pass.h"
//#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"
#include "llvm/IRBuilder.h"

#include "llvm/Support/CommandLine.h"

#include "../Tools/EnumerateCallInst.h"

using namespace llvm;

//#define MUT_DEBUG

static cl::opt<bool> 
    verbose("verbose", 
	cl::desc("Enable verbose output. "
	    "Displays filename and location of occurances"),
	cl::init(false));

// Specifies if we are in remove mode
static cl::opt<bool> rmMode("rm", 
	cl::desc("remove occurances of posix_cond_signal"),
	cl::init(false));

// Specifies if we are in replace mode
static cl::opt<bool> repMode("repmode", 
	cl::desc("replace occurance of pthread_cond_signal with pthread_cond_broadcast "
		    "or vice versa"),
	cl::init(false));

static cl::list<unsigned> PosToRm("pos", 
	cl::desc("occurances to remove or replace"),
	cl::value_desc("comma seperated list of occurances to alter"),
	cl::CommaSeparated);

namespace {
  struct PosixCondSignal : public ModulePass {
    static char ID;
    PosixCondSignal() : ModulePass(ID) { numCalls = 0; }

    EnumerateCallInst sigVis;

    unsigned numCalls;

    virtual bool runOnModule(Module &M) {
	//errs() << "Mutate Posix Signal\n";
	sigVis.addFuncNameToSearch("pthread_cond_broadcast");
	sigVis.addFuncNameToSearch("pthread_cond_signal");

	sigVis.visit(M);
	numCalls = sigVis.callInsts.size();

	bool modified;
	modified = false;

	if (rmMode && repMode) {
	    errs() << "Warning: both rmmode and repmode specified. rmmode takes precedence\n";
	}

	if (rmMode) {
	    //DEBUG(errs() << "DEBUG: in remove mode\n");

	    // Check if no position to remove were specified
	    if (PosToRm.size() == 0) {
		errs() << "Warning: In remove mode but no positions specified\n";
	    }

	    for (unsigned i = 0; i < PosToRm.size(); i++) {
		// pthread_cond_{broadcast,signal} return an int
		int ret = sigVis.removeFromParentRepZero(PosToRm[i], sizeof(int), true);
#ifdef MUT_DEBUG
		errs() << "DEBUG: remove from parent returned: " << ret << '\n';
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
	    //DEBUG(errs() << "DEBUG: in replace mode\n");
	    for (unsigned i = 0; i < PosToRm.size(); i++) {
		// Check if the specified position to remove is out of bounds of
		// the structure of found instructions
		int ret = sigVis.isValidIndex(PosToRm[i]);
		if (ret == -1) {
		    errs() << "Warning: position " << PosToRm[i] << " is out of "
			   << "bounds of found instructions, ignoring\n";
		    continue;
		}
		else if (ret == -2) {
		    errs() << "Warning: position " << PosToRm[i] << " has been "
			   << "modified already. Skipping\n";
		    continue;
		}

		CallInst *curInst;
		curInst = sigVis.callInsts[PosToRm[i]];

		//DEBUG(errs() << "DEBUG: replacing  occurence: " << PosToRm[i]
			     //<< ' ' << *curInst << '\n');

		StringRef calledFuncName;
		calledFuncName = curInst->getCalledFunction()->getName();

		if (calledFuncName == "pthread_cond_signal") {
		    if (curInst->getNumArgOperands() >= 1) {
			// The argument of pthread_cond_signal and
			// broadcast is the same
			Value *argZero = curInst->getArgOperand(0);
			Constant *c = M.getOrInsertFunction("pthread_cond_broadcast", 
			    IntegerType::get(getGlobalContext(), sizeof(int) * 8), // return type
			    argZero->getType(), NULL); // param 0
			//DEBUG(errs() << "getOrInsertFunction returned " << *c);
			curInst->setCalledFunction(c);
			ret = sigVis.markMutated(PosToRm[i]);
			if (ret) {
			    errs() << "Warning: call to markMutated returned " << ret << '\n';
			}
			modified = true;
		    }
		    else {
			errs() << "Warning: pthread_cond_signal() call "
				  "found that has 0 arguments. Skipping "
				  "replacement\n";
		    }
		}

		else if (calledFuncName == "pthread_cond_broadcast") {
		    if (curInst->getNumArgOperands() >= 1) {
			// The argument of pthread_cond_signal and
			// broadcast is the same
			Value *argZero = curInst->getArgOperand(0);
			Constant *c = M.getOrInsertFunction("pthread_cond_signal", 
			    IntegerType::get(getGlobalContext(), sizeof(int) * 8), // return type
			    argZero->getType(), NULL); // param 0
			//DEBUG(errs() << "getOrInsertFunction returned " << *c);
			curInst->setCalledFunction(c);
			sigVis.markMutated(PosToRm[i]);
			if (ret) {
			    errs() << "Warning: call to markMutated returned " << ret << '\n';
			}
			modified = true;
		    }
		    else {
			errs() << "Warning: pthread_cond_broadcast() call "
				  "found that has 0 arguments. Skipping "
				  "replacement\n";
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
	    // Print out debug information about each occurance found
	    //DEBUG(errs() << "DEBUG: Verbose mode activated" << '\n');
	    MDNode *metaNode;
	    for (unsigned i = 0; i < sigVis.callInsts.size(); i++) {
		metaNode = sigVis.callInsts[i]->getMetadata("dbg");
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
  }; // struct PosixCondSignal
} // namespace

char PosixCondSignal::ID = 0;
static RegisterPass<PosixCondSignal> X("PosixCondSignal", "mutate pthread_cond_{signal, broadcast}", false, false);
