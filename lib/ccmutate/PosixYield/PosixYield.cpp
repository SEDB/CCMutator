/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file PosixYield.cpp
 * \author Markus Kusano
 * \date 2013-02-25
 *
 * Tool to remove or find calls to {sched,posix}_yield(). See README.md for
 * more information.
 */
#include <string>
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/LLVMContext.h"

#include "../Tools/EnumerateCallInst.h"

// Enable debugging messages
#define MUT_DEBUG

using namespace llvm;
using std::string;

/// Command line option: Specify that the tool should remove occurances of
/// {sched,posix}_yield(). Positions to remove are specified with the `--pos`
/// command line option. This defaults to false, the default operation of the
/// is to print information about where occurances are.
static cl::opt<bool> rmMode("rm", 
	cl::desc("remove occurances specified by -pos"),
	cl::init(false));

/// Command line option: Comma separated list of positions to mutate. The list
/// is zero indexted (the first position is zero). \sa rmMode.
/// Note: -pos 0 -pos 3 is equivalent to -pos=0,3
static cl::list<unsigned> MutatePos("pos",
	cl::desc("occurances to mutate"),
	cl::value_desc("comma separated list of occurances to mutate"),
	cl::CommaSeparated);

/// Command line option: Verbose output boolean. Default to false. This option
/// makes sense in the default find mode (ie, when -rm is not specified).
/// Enabling this causes output to be the filename and line number of each
/// occurrence found in the form `<filename>\t<linenumber>` This requires that
/// debug metadata to exist in the LLVM bitcode (use `-g` to clang). Defaults
/// to false. 
///
/// \sa rmMode
static cl::opt<bool> verbose("verbose",
	cl::desc("enable verbose output, displays filename/linenumber info"),
	cl::init(false));

/// Checks if the commandline args are valid
void checkCommandLineArgs();

void checkCommandLineArgs() {
    // Both verbose and rm should not be specified together
    if (verbose && rmMode) {
	errs() << "Error: both verbose and rm cannot be specified together\n";
	exit(EXIT_FAILURE);
    }

    // Check if the size of positions specified is zero and we are in rmMode.
    // This does not make sense since it would be a no-op
    if (rmMode && MutatePos.size() == 0) {
	errs() << "Error: In rmMode with no positions specified to mutate\n";
	exit(EXIT_FAILURE);
    }
}


namespace {
struct PosixYield : public ModulePass {
    static char ID;

    PosixYield() : ModulePass(ID) { }

    EnumerateCallInst eci;

    virtual bool runOnModule(Module &M) {
	checkCommandLineArgs();
	bool modified; // indicates if the code has been modified
	// Enumerate instances of posix_yield and sched_yield
	eci.addFuncNameToSearch("pthread_yield");
	eci.addFuncNameToSearch("sched_yield");
	eci.visit(M);

	if (!rmMode) {
	    modified = false;
	}
	else {
	    for (unsigned i = 0; i < MutatePos.size(); i++) {
		int ret = eci.removeFromParent(MutatePos[i]);
#ifdef MUT_DEBUG
		errs() << "DEBUG: remove from parent returned: " << ret << '\n';
#endif
		if (ret == -1) {
		    errs() << "Warning: position " << MutatePos[i] << " is out of "
			   << "bounds of found instructions, ignoring\n";
		}
		else if (ret == -2) {
		    errs() << "Warning: position " << MutatePos[i] << " has been "
			   << "removed already. Skipping\n";
		}
		else {
		    modified = true;
		}
	    }
	}
	return modified;
    }

    virtual void print(llvm::raw_ostream &O, const Module *M) const {
	// Print out information (this is find mode)
	if (!verbose) {
	    errs() << eci.callInsts.size() << '\n';
	}
	else {
	    eci.printInstDebugInfo();
	}
    }

}; // struct
} // namespace

char PosixYield::ID = 0;
static RegisterPass<PosixYield> X("PosixYield", "mutate {sched,pthread}_yield()", false, false);
