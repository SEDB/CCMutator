/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file mutate_PosixSema.cpp
 * Author: Markus Kusano
 * Date: 2013-01-11
 *
 * Modify permit count on POSIX semaphores
 */

#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/DebugInfo.h"

#include "../Tools/EnumerateCallInst.h"

using namespace llvm;

/// boolean for verbose output control. Requires that the file be compiled with
/// debugging metadata
static cl::opt<bool> verbose("v", 
	cl::desc("Enable verbose output. Display filename and line number of occurrences"),
	cl::init(false));

static cl::opt<bool> modify("mod", 
	cl::desc("Enable modify mode"),
	cl::init(false));

static cl::list<unsigned> posToMod("pos",
	cl::desc("Positions to modify"),
	cl::value_desc("Comma seperated list of positions to modify"),
	cl::CommaSeparated);

static cl::list<unsigned> valToMod("val",
	cl::desc("Values to modify with"),
	cl::value_desc("Comma seperated list of values to use to modify respective positions"),
	cl::CommaSeparated);

static void printErrMsg(int err, unsigned index);

void printErrMsg(int err, unsigned index) {
    if (err == -1) {
	errs() << "Warning: Attempting to mutate instruction at position " 
	       << index << " which is out-of-bounds of found instructions,"
	       << "skipping\n";
    }
    else if (err == -2) {
	errs() << "Warning: Attempting to mutate instruction at position "
	       << index << " that has already been mutated, skipping\n";
    }
    else {
	errs() << "Warning: An undefined error occurred\n";
    }
}


namespace {
  struct mutate_PosixSema: public ModulePass {
    static char ID;
    mutate_PosixSema() : ModulePass(ID) { }

    EnumerateCallInst semVis;

    virtual bool runOnModule(Module &M) {
	bool modified;
	modified = false;

	errs() << "SEM_VALUE_MAX == " << SEM_VALUE_MAX << '\n';

	errs() << "mutate_PosixSema: \n";
	semVis.addFuncNameToSearch("sem_open");
	semVis.addFuncNameToSearch("sem_init");
	semVis.visit(M);
	DEBUG(errs() << "DEBUG: Found " << semVis.callInsts.size()
		     << " instances of calls to sema permit count modifying calls\n");

	if (modify) {
	    DEBUG(errs() << "DEBUG: in modify mode\n");

	    srand(time(NULL));

	    if (posToMod.size() == 0) {
		errs() << "Warning: in modify mode with no positions to modify specified\n";
	    }

	    for (unsigned i = 0; i < posToMod.size(); i++) {
		int ret = semVis.isValidIndex(posToMod[i]);
		if (ret == -1) {
		    printErrMsg(-1, posToMod[i]);
		    continue;
		}
		else if (ret == -2) {
		    printErrMsg(-2, posToMod[i]);
		    continue;
		}

		// Calculate the new value to modify the current instruction
		ConstantInt *newVal;
		if (i >= valToMod.size()) {
		    errs() << "Warning: No value to modify for instruction. "
			      "using rand()\n";
		    newVal = ConstantInt::get(IntegerType::get(getGlobalContext(), 
				sizeof(unsigned) * 8), rand() % SEM_VALUE_MAX, false);
		}
		else if (valToMod[i] > SEM_VALUE_MAX) {
		    errs() << "Warning: Value to modify greater than SEM_VALUE_MAX, "
			      "setting to SEM_VALUE_MAX\n";
		    newVal = ConstantInt::get(IntegerType::get(getGlobalContext(), 
				sizeof(unsigned) * 8), SEM_VALUE_MAX, false);
		}
		else {
		    newVal = ConstantInt::get(IntegerType::get(getGlobalContext(), 
			sizeof(unsigned) * 8), valToMod[i], false);
		}

		// Modify the sem_init or sem_open value parameter
		CallInst *curInst = semVis.callInsts[posToMod[i]];
		if (curInst->getCalledFunction()->getName() == "sem_init") {
		    // Third paramenter of sem_init is the permit value
		    if (curInst->getNumArgOperands() < 3) {
			errs() << "Warning: call to sem_init found with less than "
				  " 3 arguments, skipping\n";
			continue;
		    }
		    curInst->setArgOperand(2, newVal);
		    ret = semVis.markMutated(posToMod[i]);
		    if (ret == -1) {
			printErrMsg(-1, posToMod[i]);
			continue;
		    }
		    else if (ret == -2) {
			printErrMsg(-2, posToMod[i]);
			continue;
		    }
		    modified = true;
		}
		// OPTIMIZATION: The check for the name is not necessary since
		// it is the only other option. However, more semaphore
		// functions might be added.
		else if (curInst->getCalledFunction()->getName() == "sem_open") {
		    // SemaVisitor should have already filtered out sem_open()
		    // calls that do not have 4 arguments
		    if (curInst->getNumArgOperands() < 4) {
			errs() << "Warning: attempting to modify call to sem_open "
				  "with less than 4 arguments, skipping\n";
			continue;
		    }
		    ret = semVis.markMutated(posToMod[i]);
		    if (ret == -1) {
			printErrMsg(-1, posToMod[i]);
			continue;
		    }
		    else if (ret == -2) {
			printErrMsg(-2, posToMod[i]);
			continue;
		    }
		    curInst->setArgOperand(3, newVal);
		    modified = true;
		}
	    }
	}
	return modified;
    }

    virtual void print(llvm::raw_ostream &O, const Module *M) const {
	if (!verbose) {
	    errs() << semVis.callInsts.size() << '\n';
	}
	else {
	    DEBUG(errs() << "DEBUG: Verbose mode activated" << '\n');
	    // Iterate over each call instruction and print out
	    // it's debugging information.
	    MDNode *metaNode;
	    for (unsigned i = 0; i < semVis.callInsts.size(); i++) {
		metaNode = semVis.callInsts[i]->getMetadata("dbg");
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
  }; // struct mutate_PosixSema
} // namespace


char mutate_PosixSema::ID = 0;
static RegisterPass<mutate_PosixSema> X("PosixSema", "mutate semaphore value modify calls", false, false);
