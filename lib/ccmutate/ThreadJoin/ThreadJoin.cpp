/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file ThreadJoin.cpp
 * Author: Markus Kusano
 * Date: 2013-01-11
 *
 * See README.md for more info
 */

#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"
#include "llvm/IRBuilder.h"

#include "llvm/Support/CommandLine.h"

#include "../Tools/EnumerateCallInst.h"
#include "../Tools/ItaniumDemangle.h"

using namespace llvm;

#define MUT_DEBUG

// Command line flags
static cl::opt<bool> 
    verbose("v", 
	cl::desc("Enable verbose output."
	    "Displays filename and location of occurances"),
	cl::init(false));

static cl::opt<bool> posix("posix",
        cl::desc("Enable mutation of POSIX thread calls"),
        cl::init(false));

static cl::opt<bool> cxx11("c++11",
        cl::desc("Enable mutation of C++11 thread calls"),
        cl::init(false));

// Specifies if we are in remove mode
static cl::opt<bool> rmMode("rm", 
	cl::desc("remove occurances of join"),
	cl::init(false));

// Specifies if we are in replace join with sleep mode
static cl::opt<bool> repMode("rep", 
	cl::desc("replace occurance of join with sleep"),
	cl::init(false));

// Command line option that specifies which occurance of pthread_join should be
// removed. Use the analysis FindPosixJoin to get the enumeration. Specify a
// comma seperated list of 0 indexed values (e.g. -pos=1,3,4) which will
// remove occurances 1, 3 and 4.
static cl::list<unsigned> PosToRm("pos", 
	cl::desc("occurances to remove or replace with sleep"),
	cl::value_desc("comma seperated list of occurances to alter"),
	cl::CommaSeparated);

/// TODO: This could be a list so that different values to sleep could be
/// passed for different selected occurrences.
static cl::opt<unsigned> SleepValue("sleepval",
	cl::desc("value to be passed to call to sleep, default is 1"),
	cl::value_desc("positive integer"),
	cl::init(1));

namespace {
  struct ThreadJoin: public ModulePass {
    static char ID;
    ThreadJoin() : ModulePass(ID) { numCalls = 0; }

    /**
     * Visitor to find CallInst or Invokes to join
     */
    EnumerateCallInst pjv;

    /**
     * Number of calls to join found
     */
    unsigned numCalls;

    /**
     * Uses a FindPosixJoinVisitor to find occurances of CallInst to
     * pthread_join
     */
    virtual bool runOnModule(Module &M) {
	errs() << "FindPosixJoin: \n";

        checkCommandLineArgs();

        if (posix) {
	    pjv.addFuncNameToSearch("pthread_join");
        }
        if (cxx11) {
	    pjv.addFuncNameToSearch("std::__1::thread::join");
            pjv.searchCpp();
        }

	pjv.visit(M);

#ifdef MUT_DEBUG
        errs() << "DEBUG: found " << pjv.callInsts.size() << " CallInsts and " 
               << pjv.invokeInsts.size() << " InvokeInsts\n";
#endif

        // Sum up all the instructions found
	numCalls = pjv.callInsts.size() + pjv.invokeInsts.size();

#ifdef MUT_DEBUG
        errs() << "DEBUG: cxx11 == " << cxx11 << "\n\tposix== " << posix << '\n';
#endif


	bool modified; /**< Indicates if the program was modified */
	modified = false;


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
            errs() << "PosToRm.size() == " << PosToRm.size() << '\n';
#endif
	    for (unsigned i = 0; i < PosToRm.size(); i++) {
		int errorCode;
                bool isThreadJoin;
                bool isCallInst;
                Instruction *curInst = pjv.getInstructionAt(PosToRm[i], &isCallInst, &errorCode);
		if (curInst == NULL) {
		    if (errorCode == -1) {
			errs() << "Warning: attempting to modify instruction out-of-bounds "
				  " of found instructions, skipping\n";
		    }
		    else if (errorCode == -2) {
			errs() << "Warning: attempting to modify the same instruction twice, "
				  "skipping\n";
		    }
                    else {
                        errs() << "Warning: in ThreadJoin, unknown return code "
                                  "from pjv.isValidIndex()\n";
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
		IRBuilder<> builder(curInst);
		Constant *c = M.getOrInsertFunction("sleep", 
			IntegerType::get(getGlobalContext(), sizeof(unsigned) * 8),
			IntegerType::get(getGlobalContext(), sizeof(unsigned) * 8), NULL);
		CallInst *sleepCall = CallInst::Create(c, args, "sleep_mut");
                isThreadJoin = isStdThreadJoin(curInst, isCallInst);
#ifdef MUT_DEBUG
                errs() << "DEBUG: isThreadJoin == " << isThreadJoin << '\n';
#endif
                int ret;
                if (!isThreadJoin) {
                    // The return type of pthread_join and sleep() is the same
                    // so they can be replaced blindly regardless of their uses
		    ret = pjv.replaceInstWithInst(PosToRm[i], sleepCall);
                }
                else {
                    ret = pjv.markMutated(PosToRm[i]);
#ifdef MUT_DEBUG
                    errs() << "DEBUG: Replacing occurrance of std::thread::join\n";
                    errs() << "\tpjv.markMutated return code == " << ret << '\n';
#endif
                    if (ret >= 0) {
                        // markMutated returns >=0 if it is a valid index and
                        // uses the same return codes as replaceInstWithInst
                        
                        // Insert a call to sleep before the std::thread::join call
                        // and then replace the invoke inst with a branch to the
                        // normal destination
                        BasicBlock *curBB;
                        curBB = curInst->getParent();   // parent of instruction is BB
                        curBB->getInstList().insert(curInst, sleepCall);    // insert before

                        BasicBlock *normDest;
                        BranchInst *normBranch;
                        normDest = ((InvokeInst *)curInst)->getNormalDest();
                        normBranch = BranchInst::Create(normDest);
                        curBB->getInstList().insert(curInst, normBranch);

                        if (curInst->hasNUses(0)) {
                            curInst->eraseFromParent();
                        }
                        else {
                            errs() << "Warning: encountered std::thread::join call that "
                                      "still has uses but is being removed, skipping mutation\n"
                                      "Note: the module is likely broken, sorry\n";
                        }
                    }
                }

                // TODO: We check for these errors twice and all over the code.
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

    void checkCommandLineArgs() {
        if (!(cxx11 || posix)) {
            errs() << "Error: atleast -posix or -c++11 must be specified\n";
            exit(EXIT_FAILURE);
        }
	// Check if both rm and replace were specified.
	if (rmMode && repMode) {
	    errs() << "Error: both remove mode and replace mode were specified\n";
	    exit(EXIT_FAILURE);
	}

        // Verbose has no meaning in rm or replace
        if (rmMode && verbose) {
            errs() << "Error: -v has no meaning with -rm\n";
            exit(EXIT_FAILURE);
        }
        if (repMode && verbose) {
            errs() << "Error: -v has no meaning with -rep\n";
            exit(EXIT_FAILURE);
        }

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
	    for (unsigned i = 0; i < pjv.invokeInsts.size(); i++) {
		metaNode = pjv.invokeInsts[i]->getMetadata("dbg");
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
    // Returns true if the passed instruction demangles to some kind of call to
    // std::__1::thread::join. Requires a bool to specify if the pointer is of
    // type CallInst or InvokeInst.
    bool isStdThreadJoin(Instruction *inst, bool isCallInst) {
        Function *calledFunc;
        StringRef funcName;
        if (isCallInst) {
            calledFunc = ((CallInst *)inst)->getCalledFunction();
        }
        else {
            calledFunc = ((InvokeInst *)inst)->getCalledFunction();
        }
        funcName = calledFunc->getName();
        char *demangled = demangleCpp(funcName);

        if (demangled == NULL) {
            return false;
        }
        std::string noParams;
        noParams = removeParameters(demangled);

        free(demangled);

        if (noParams == "std::__1::thread::join") {
            return true;
        }
        return false;
    }


  }; // struct
} // namespace

char ThreadJoin::ID = 0;
static RegisterPass<ThreadJoin> X("ThreadJoin", "mutate thread synchronization join calls", false, false);
