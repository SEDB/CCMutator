/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file PosixCondWait.cpp
 * \author Markus Kusano
 * \date 2013-02-28
 *
 * Implementation of a mutator that works on pthread_cond_wait() and
 * pthread_cond_timed_wait()
 *
 * See README.md for more details
 */
#include "llvm/Support/CommandLine.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "../Tools/EnumerateCallInst.h"

#define MUT_DEBUG

/// The location in timespec structure of the field tv_sec. This is included
/// because POSIX does not make guaranetees where in the strucutre the element
/// is but only that it is in there somewhere.
static int tv_sec_loc = 0;

/// The location in timespec structure of the field tv_nsec. This is included
/// because POSIX does not make guaranetees where in the strucutre the element
/// is but only that it is in there somewhere.
static int tv_nsec_loc = 1;


using namespace llvm;

/// Command line option: specify if we should remove occurrences specified with
/// -pos. Defaults to false. \sa MutatePos
static cl::opt<bool> rmMode("rm", 
	cl::desc("remove occurrences specified by -pos"),
	cl::init(false));

/// Command line option: specify if we should modify occurrences specified with
/// -pos. Defaults to false. This allows for the wait time to be set to a
/// certain value. If the instruction is a call to pthread_cond_wait it is
/// replaced with a call to pthread_cond_timed_wait with the given time value.
/// Speicfy the value with -val. If no value is specified, a random one is
/// used.
///
/// \sa MutatePos 
/// \sa ModVal
static cl::opt<bool> TimeMod("tmod", 
	cl::desc("modify wait value of occurrences specified by -val"),
	cl::init(false));

/// Command line option: Comma separated list of positions to mutate. The list
/// is zero indexted (the first position is zero). \sa rmMode.
/// Note: -pos 0 -pos 3 is equivalent to -pos=0,3
static cl::list<unsigned> MutatePos("pos",
	cl::desc("occurances to mutate"),
	cl::value_desc("comma separated list of occurances to mutate"),
	cl::CommaSeparated);

/// Command line option: Comma separated list of values to be used when
/// mutating the time wait tv_nsec value. The 0th position in this list will be
/// the value used for the 0th instruction specified by -pos. If this list is
/// shorter than -pos then the last value in the list will be used for the
/// remaining positions to modify.
static cl::list<int> NsecModVal("nsecval",
	cl::desc("modify values"),
	cl::value_desc("comma separated list of values to use"),
	cl::CommaSeparated);

/// Command line option: Comma separated list of values to be used when
/// mutating the time wait tv_sec value. The 0th position in this list will be
/// the value used for the 0th instruction specified by -pos. If this list is
/// shorter than -pos then the last value in the list will be used for the
/// remaining positions to modify.
static cl::list<int> SecModVal("secval",
	cl::desc("modify values"),
	cl::value_desc("comma separated list of values to use"),
	cl::CommaSeparated);

/// Command line option: Comma separated list of values to be specify the
/// instruction relative to the call to the start of the function containing
/// the call to pthread_cond_timedwait that is being modified. All mutation
/// code is inserted before the specified instruction. If this list is shorter
/// than -pos then the last value in the list will be used for the remaining
/// position to modify.
static cl::list<unsigned> InsertPoint("inspt",
	cl::desc("relative point to insert mutation code"),
	cl::value_desc("comma separated list of ints"),
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

/// Command line option: Tells the pass to switch calls specified with -pos
/// from timedwait to wait. The converse (wait to timedwait) is not implemented
/// yet.
static cl::opt<bool> switchMode("switch",
	cl::desc("switch calls to timedwait to wait"),
	cl::init(false));

/// Verifies command line arguments are valid
void checkCommandLineArgs();

/// Obtains the next second mutate and nsec mutate values from the nsecval and
/// secval lists.
/// \param secVal next second mutation value (returned)
/// \param nsecVal next nano second mutation value (returned)
/// \param timedWait the call to pthread_cond_timedwait
/// \param pos next position being mutated
/// \return Instruction of the next insertion point
Instruction *getNextMutateVals(int &secVal, int &nsecVal, CallInst *timedWait, unsigned pos);

/// Returns an instruction poitner that corresponds to the val'th instruction
/// in the passed function. 
/// \param F function to iterate over
/// \param val number of times to move the inst_iterator
/// \return Instruction* 
Instruction *getInstFromFunction(Function *F, unsigned val);

void checkCommandLineArgs() {
    // Both verbose and rm should not be specified together
    if (verbose && rmMode) {
	errs() << "Error: both verbose and rm cannot be specified together\n";
	exit(EXIT_FAILURE);
    }

    // Modify and remove mode should not be specified together
    if (TimeMod && rmMode) {
	errs () << "Error: both -rm and -tmod cannot be specified together\n";
	exit(EXIT_FAILURE);
    }

    if (TimeMod && switchMode) {
	errs() << "Error: both -tmod and -swtich cannot be specified together\n";
	exit(EXIT_FAILURE);
    }

    if (rmMode && switchMode) {
	errs() << "Error: both -rm and -switch cannot be specified together\n";
	exit(EXIT_FAILURE);
    }

    // Check if the size of positions specified is zero and we are in rmMode.
    // This does not make sense since it would be a no-op
    if (rmMode && MutatePos.size() == 0) {
	errs() << "Error: In rmMode with no positions specified to mutate\n";
	exit(EXIT_FAILURE);
    }

    if (switchMode && MutatePos.size() == 0) {
	errs() << "Error: -switch set but no positions specified (see -pos)\n";
	exit(EXIT_FAILURE);
    }

    // Check if the size of positions specified is zero and we are in time
    // mutate mode.  This does not make sense since it would be a no-op
    if (TimeMod && MutatePos.size() == 0) {
	errs() << "Error: In time mutate mode with no positions specified to mutate\n";
	exit(EXIT_FAILURE);
    }

    if (TimeMod && SecModVal.size() == 0) {
	errs() << "Error: In time mutate mode with no second values specified\n";
	exit(EXIT_FAILURE);
    }

    if (TimeMod && NsecModVal.size() == 0) {
	errs() << "Error: In time mutate mode with no nano second values specified\n";
	exit(EXIT_FAILURE);
    }

    if (switchMode && SecModVal.size() != 0) {
	errs() << "Error: -switch does not use -secval\n";
	exit(EXIT_FAILURE);
    }
    if (switchMode && NsecModVal.size() != 0) {
	errs() << "Error: -switch does not use -nsecval\n";
	exit(EXIT_FAILURE);
    }
}

Instruction *getInstFromFunction(Function *F, unsigned val) {
    inst_iterator I = inst_begin(F);
    inst_iterator end = inst_end(F);

    for (unsigned i = 0; (I != end) && (i != val); ++I, ++i) {
	// Empty body
    }

    return &(*I);
}

Instruction *getNextMutateVals(int &secVal, int &nsecVal, CallInst *timedWait, unsigned pos) {
    Instruction *insPoint = timedWait; // default return value

    if (pos >= NsecModVal.size()) {
#ifdef MUT_DEBUG
	errs() << "DEBUG: NsecModVal list shorter than pos list, using last value\n";
#endif
	nsecVal = NsecModVal[NsecModVal.size() -1];
    }
    else {
	nsecVal = NsecModVal[pos];
    }

    if (pos >= SecModVal.size()) {
#ifdef MUT_DEBUG
	errs() << "DEBUG: SecModVal list shorter than pos list, using last value\n";
#endif
	secVal = SecModVal[NsecModVal.size() -1];
    }
    else {
	secVal = SecModVal[pos];
    }

    if (pos >= InsertPoint.size()) {
	if (InsertPoint.size() != 0) {
	    // use the last value for the remaining positions
	    insPoint = getInstFromFunction(timedWait->getParent()->getParent(), 
		    InsertPoint[InsertPoint.size() - 1]);
	}
	// if InsertPoint.size() == 0 then leave insPoint unmodified
    }
    else {
	insPoint = getInstFromFunction(timedWait->getParent()->getParent(),
	    InsertPoint[pos]);
    }

#ifdef MUT_DEBUG
    errs() << "DEBUG: mutation values for pos " << pos << '\n'
	   << "\tsecVal == " << secVal << '\n'
	   << "\tnsecVal == " << nsecVal << '\n'
	   << "\tinsPt == " << *insPoint << '\n';
#endif

    return insPoint;
}

namespace {
struct PosixCondWait : public ModulePass {

    static char ID;

    PosixCondWait() : ModulePass(ID) { }

    EnumerateCallInst eci;

    virtual bool runOnModule(Module &M) {
	checkCommandLineArgs();
	bool modified = false; // indicates if the code has been modified
	// Enumerate instances of call instructions to mutate
	eci.addFuncNameToSearch("pthread_cond_wait");
	eci.addFuncNameToSearch("pthread_cond_timedwait");
	eci.visit(M);

	if (!rmMode && !TimeMod &&!switchMode) {
	    modified = false; // implicit print mode
	}
	else if (rmMode){
	    for (unsigned i = 0; i < MutatePos.size(); i++) {
		int ret = eci.removeFromParentRepZero(MutatePos[i], sizeof(int), true);
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
	else if (TimeMod) {
#ifdef MUT_DEBUG
	    errs() << "DEBUG: in modify mode\n";
#endif

	    for (unsigned i = 0; i < MutatePos.size(); i++) {
		unsigned posToMod = MutatePos[i];

		if (posToMod >= eci.callInsts.size()) {
		    errs() << "Warning: Position " << MutatePos[i] 
			   << " to modify out of bounds, skipping\n";
		    continue; // go to next mutate position
		}

		int secMod;
		int nsecMod;
		CallInst *curInst = eci.callInsts[posToMod];
		Instruction *insPoint;
		insPoint = getNextMutateVals(secMod, nsecMod, curInst, MutatePos[i]);

		if (curInst->getCalledFunction()->getName() == "pthread_cond_timedwait") {
		    // Attempt to get the type of struct timespec
		    StructType *timespecType = M.getTypeByName("struct.timespec");
		    if (timespecType == NULL) {
			errs() << "Warning: unable to find definition of type "
				  "struct.timespec, skipping modifcation\n";
			continue;
		    }

		    IRBuilder<> builder(insPoint);
		    Value *timespecStruct;

		    if (curInst->getNumArgOperands() < 3) {
			errs() << "Warning: found pthread_cond_timedwait() call "
			          "with less than 3 arguments, skipping\n";
			continue; // go to next mutate position
		    }

		    // Pointer to timespec used in CallInst
		    timespecStruct = curInst->getArgOperand(2);

		    // Pointer to first element of timespec struct (tv_sec)
		    Value *tvSecPtr = builder.CreateConstGEP2_32(timespecStruct, 0, tv_sec_loc, "tv_sec_mut");

		    // Pointer to second element of timespec struct (tv_nsec)
		    Value *tvNsecPtr = builder.CreateConstGEP2_32(timespecStruct, 0, tv_nsec_loc, "tv_nsec_mut");

		    // Load both pointers values so they can be modified
		    Value *tvSecVal = builder.CreateLoad(tvSecPtr, "tvSecVal_mut");
		    Value *tvNsecVal = builder.CreateLoad(tvNsecPtr, "tvNsecVal_mut");

		    // Create modification value for seconds. The type is
		    // time_t which is opaque and must be obtained
		    if (timespecType->getNumElements() < 2) {
			errs() << "Warning: found struct.timespec type with "
				  "less than 2 elements, skipping modification\n";
			continue;   // go to next mutate position
		    }

		    Value *secModVal = ConstantInt::getSigned(timespecType->getElementType(tv_sec_loc), secMod);
		    Value *nsecModVal = ConstantInt::getSigned(timespecType->getElementType(tv_nsec_loc), nsecMod);

		    // Perform modifcations (result = tv_sec + secMod)
		    // No checking for overflow is done
		    Value *secAddRes = builder.CreateAdd(tvSecVal, secModVal, "sec_mod_val");
		    Value *nsecAddRes = builder.CreateAdd(tvNsecVal, nsecModVal, "nsec_mod_val");

		    // Store the results back
		    builder.CreateStore(secAddRes, tvSecPtr);
		    builder.CreateStore(nsecAddRes, tvNsecPtr);
		}
		else {
		    errs() << "Warning: attempting to mutate the timespec value "
			      "of a call other than pthread_cond_timedwait(), skipping\n";
		}
	    }

	    modified = true;
	} // end else if
	else if (switchMode) {
#ifdef MUT_DEBUG
	    errs() << "DEBUG: in switch mode\n";
#endif
	    for (unsigned i = 0; i < MutatePos.size(); i++) {
		unsigned posToMod = MutatePos[i];

		if (posToMod >= eci.callInsts.size()) {
		    errs() << "Error: position " << posToMod << " is out-of-bounds "
			      "of found instructions, skipping\n";
		    continue;
		}

		CallInst *curCall = eci.callInsts[posToMod];
		Function *curFunc = curCall->getCalledFunction();

		if (!curFunc) {
		    errs() << "Warning: indirect function call found, skipping\n";
		    continue;
		}

		if (curFunc->getName() == "pthread_cond_wait") {
		    errs() << "Warning: switching pthread_cond_wait to pthread_cond_timedwait "
			      "is not supported, skipping\n";
		    continue;
		}

		if (curCall->getNumArgOperands() != 3) {
		    errs() << "Warning: found call to pthread_cond_timedwait that "
			      "does not have 3 operands, skipping\n";
		    continue;
		}

		// Create a new call to pthread_cond_wait with the same
		// arguments except drop the third
		Value *args[2];
		args[0] = curCall->getArgOperand(0);
		args[1] = curCall->getArgOperand(1);

		Constant *pthreadCondWait = M.getOrInsertFunction("pthread_cond_wait", 
		    IntegerType::get(getGlobalContext(), sizeof(int) * 8), // return type
		    args[0]->getType(), // param 0
		    args[1]->getType(), // param 1
		    NULL);

		CallInst *newCall;
		newCall = CallInst::Create(pthreadCondWait, args, "mut_timedWait");

		BasicBlock::iterator iter(curCall);
		ReplaceInstWithInst(curCall->getParent()->getInstList(), iter, newCall);

		modified = true;
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

char PosixCondWait::ID = 0;
static RegisterPass<PosixCondWait> X("PosixCondWait", "mutate "
	"pthread_cond_wait() or _timed_wait()", false, false);
