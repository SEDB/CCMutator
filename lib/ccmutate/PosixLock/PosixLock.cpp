/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file PosixLock.cpp
 * \author Markus Kusano
 * \date 2013-02-25
 *
 * Tool to remove or find calls to {sched,posix}_yield(). See README.md for
 * more information.
 */
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "../Tools/RemoveInst.h"

#include "llvm/Support/InstIterator.h"

#include "../Tools/LockUnlockPairs.h"

#include <algorithm> // std::sort and std::unique

// Enable debugging messages
#define MUT_DEBUG

using namespace llvm;

/// Command line option: Verbose output boolean. Default to false. This option
/// makes sense in the default find mode (ie, when -rm is not specified).
/// Defaults to false
///
/// \sa rmMode
static cl::opt<bool> verbose("verbose",
	cl::desc("enable verbose output, displays filename/linenumber info"),
	cl::init(false));

/// Command line option: the pass will remove pairs specified by -pos
static cl::opt<bool> rmMode("rm",
	cl::desc("enable remove mode, remove lock unlock pair specified by pos\n"),
	cl::init(false));

/// Command line option: the pass will swap pairs specified by -pos. This
/// requires atleast 4 values to be speciied in -pos
static cl::opt<bool> swapMode("swap",
	cl::desc("enable swap mode, swap lock unlock pairs specified by pos\n"),
	cl::init(false));

/// Command line option: positions to mutate. Pairs are setup as a map in the
/// form (function index, pair index) which means -pos accepts pairs of values:
/// the first one is the function index and the second is the pair. For example
/// -pos=1,2 -pos=0,7 will specify function 1 pair 2 and function 0 pair 7 to
/// be mutated. This is equivalent to -pos=1,2,0,7.
static cl::list<unsigned> MutatePos("pos",
	cl::desc("occurances to mutate"),
	cl::value_desc("comma separated list of occurances to mutate"),
	cl::CommaSeparated);

/// Command line option: enables shift mode. This allows -lockdir and
/// -unlockdir to be used in conjunction with -pos to shift pairs arbitrary
/// amounts.
static cl::opt<bool> shiftMode("shift",
	cl::desc("enable shift mode, shift lock and unlock calls"),
	cl::init(false));

/// Command line option: used in shift mutations. This is the direction to
/// shift the lock call. Positive indicates a shift downard (to a higher line
/// number) and neagive index indiactes a shift upward (to a lower line number)
/// Each index in this list corresponds to a pair specified by -pos. If no
/// value is specified for a pair (ie this list is shorter than -pos) then 0 is
/// used.
static cl::list<int> LockDir("lockdir",
	cl::desc("direction to shift lock call"),
	cl::value_desc("comma separated list of directions for each mutation position"),
	cl::CommaSeparated);

/// Command line option: used in shift mutations. This is the direction to
/// shift the unlock call. Positive indicates a shift downard (to a higher line
/// number) and neagive index indiactes a shift upward (to a lower line number)
/// Each index in this list corresponds to a pair specified by -pos. If no
/// value is specified for a pair (ie this list is shorter than -pos) then 0 is
/// used.
static cl::list<int> UnlockDir("unlockdir",
	cl::desc("direction to shift unlock call"),
	cl::value_desc("comma separated list of directions for each mutation position"),
	cl::CommaSeparated);

/// Command line option: used to specify the split locations when -split is
/// used. The input is similar to -pos in that it accepts a pair of unsigned
/// integers for each pair specified with -pos. The values specify positions
/// relative to the lock call in which the additional unlock and lock cal
/// should be inserted. -splitpos=3,7 inserts a call to unlock 3 instructions
/// down and a call to lock 7 instructions down.
static cl::list<unsigned> SplitPos("splitpos",
	cl::desc("relative position to insert unlock and lock call to split a pair"),
	cl::value_desc("comma separated list of pairs for each mutation position"),
	cl::CommaSeparated);

/// Command line option: used to specify that the pass should split a
/// lock-unlock pair. Pairs are specified with -pos and the split points are
/// specified with -splitpos
static cl::opt<bool> splitMode("split",
	cl::desc("enable split mode, split a lock and unlock pair"),
	cl::init(false));


namespace {
struct RmLockPair : public ModulePass {
    static char ID;

    RmLockPair() : ModulePass(ID) { }

    LockUnlockPairs lockPairs;

    // Vector of instructions to mutate. This is used with std::sort and
    // std::unique to keep only one occurrence of each instruction to be
    // mutated. This is done so that the same mutation operator is not
    // preformed on the same instruction more than once (eg the instruction
    // gets removed twice).
    SmallPtrSet<CallInst *, 64> mutateInsts;
    

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<AliasAnalysis>();
	AU.addPreserved<AliasAnalysis>();
    }

    virtual bool runOnModule(Module &M) {
	bool modified; // indicates if the code has been modified
	AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
	lockPairs.visit(M, AA);

	modified = false;

	checkCommandLineArgs();

	if (rmMode) {
#ifdef MUT_DEBUG
	    errs() << "DEBUG: In rmMode\n";
#endif
	    LockUnlockPairs::lockUnlockPair *curPair;
	    for (unsigned i = 0; i < MutatePos.size(); i += 2) {
		curPair = lockPairs.getPair(MutatePos[i], MutatePos[i+1]);
		if (!curPair) {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") is out of bounds, skipping\n";
		    continue;
		}
#ifdef MUT_DEBUG
		errs() << "DEBUG: adding to remove: " << *(curPair->lockCall) << '\n';
		errs() << "DEBUG: adding to remove: " << *(curPair->unlockCall) << '\n';
#endif
		mutateInsts.insert(curPair->lockCall);
		mutateInsts.insert(curPair->unlockCall);
	    }

#ifdef MUT_DEBUG
	    errs() << "DEBUG: instructions to remove:\n";
	    errs() << "\tsize == " << mutateInsts.size() << '\n';
	    SmallPtrSet<CallInst *, 64>::iterator Id = mutateInsts.begin();
	    SmallPtrSet<CallInst *, 64>::iterator Ed = mutateInsts.end();

	    for (; Id != Ed; ++Id) {
		errs() << "\t" << **Id << '\n';
	    }
#endif
	    if (mutateInsts.size() > 0) {
		modified = true;
	    }

	    // Remove each instruction or replace it with zero if it has uses
	    SmallPtrSet<CallInst *, 64>::iterator I = mutateInsts.begin();
	    SmallPtrSet<CallInst *, 64>::iterator E = mutateInsts.end();
	    for (;I != E; ++I) {
		eraseFromParentOrReplace(*I, 0, sizeof(int), true);
	    }
	}
	else if (swapMode) {
#ifdef MUT_DEBUG
	    errs() << "DEBUG: in swapMode\n";
#endif
	    LockUnlockPairs::lockUnlockPair *pair1;
	    LockUnlockPairs::lockUnlockPair *pair2;
	    for (unsigned i = 0; i < MutatePos.size(); i += 4) {
		pair1 = lockPairs.getPair(MutatePos[i], MutatePos[i+1]);
		pair2 = lockPairs.getPair(MutatePos[i+2], MutatePos[i+3]);
		if (!pair1) {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") is out of bounds, skipping\n";
		    continue;
		}
		if (!pair2) {
		    errs() << "Warning: position pair (" << MutatePos[i+2] << ' '
			   << MutatePos[i+3] << ") is out of bounds, skipping\n";
		    continue;
		}

#ifdef MUT_DEBUG
		errs() << "DEBUG: swapping:\n"
		       << '\t' << *(pair1->lockCall) << '\n'
		       << '\t' << *(pair1->unlockCall) << '\n'
		       << "with\n"
		       << '\t' << *(pair2->lockCall) << '\n'
		       << '\t' << *(pair2->unlockCall) << '\n';
#endif

		// Check and see if the user is trying to swap a pair that
		// stems from the same lock call
		if (pair1->lockCall == pair2->lockCall) {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") and position pair (" 
			   << MutatePos[i+2] << ' ' << MutatePos[i+3] 
			   << ") stem from the same lock call, skipping\n";
		    continue;
		}

		// Create a copy of all the lock and unlock instructions
		CallInst *lock1;
		CallInst *unlock1;
		CallInst *lock2;
		CallInst *unlock2;

		if (pair1->lockCall->getNumArgOperands() < 1) {
		    errs() << "Warning: lock call found with less than one arg operand, skipping\n";
		    continue;
		}
		if (pair1->unlockCall->getNumArgOperands() < 1) {
		    errs() << "Warning: unlock call found with less than one arg operand, skipping\n";
		    continue;
		}
		if (pair2->lockCall->getNumArgOperands() < 1) {
		    errs() << "Warning: lock call found with less than one arg operand, skipping\n";
		    continue;
		}
		if (pair2->unlockCall->getNumArgOperands() < 1) {
		    errs() << "Warning: unlock call found with less than one arg operand, skipping\n";
		    continue;
		}

		modified = true; 

		ArrayRef<Value *> lock1Args(pair1->lockCall->getArgOperand(0));
		ArrayRef<Value *> unlock1Args(pair1->unlockCall->getArgOperand(0));
		ArrayRef<Value *> lock2Args(pair2->lockCall->getArgOperand(0));
		ArrayRef<Value *> unlock2Args(pair2->unlockCall->getArgOperand(0));

		lock1 = CallInst::Create(pair1->lockCall->getCalledFunction(), 
			    lock1Args, "mut_lock1");
		unlock1 = CallInst::Create(pair1->unlockCall->getCalledFunction(), 
			    unlock1Args, "mut_unlock1");
		lock2 = CallInst::Create(pair2->lockCall->getCalledFunction(), 
			    lock2Args, "mut_lock2");
		unlock2 = CallInst::Create(pair2->unlockCall->getCalledFunction(), 
			    unlock2Args, "mut_unlock2");

#ifdef MUT_DEBUG
		errs() << "DEBUG: Performing replacment\n";
		errs() << "DEBUG: Begin dump of copied instructions\n";
		errs() << '\t' << *lock1 << '\n';
		errs() << '\t' << *unlock1 << '\n';
		errs() << '\t' << *lock2 << '\n';
		errs() << '\t' << *unlock2 << '\n';

#endif
		BasicBlock::iterator iter1(pair1->lockCall);
		ReplaceInstWithInst(pair1->lockCall->getParent()->getInstList(),
			iter1, lock2);

		BasicBlock::iterator iter2(pair1->unlockCall);
		ReplaceInstWithInst(pair1->unlockCall->getParent()->getInstList(),
			iter2, unlock2);

		BasicBlock::iterator iter3(pair2->lockCall);
		ReplaceInstWithInst(pair2->lockCall->getParent()->getInstList(),
			iter3, lock1);

		BasicBlock::iterator iter4(pair2->unlockCall);
		ReplaceInstWithInst(pair2->unlockCall->getParent()->getInstList(),
			iter4, unlock1);
	    } // end for
	} // end else if

	else if (shiftMode) {
#ifdef MUT_DEBUG
	    errs() << "DEBUG: in shift mode\n";
#endif
	    // checkCommandLineArgs() guarantees this will have atleast two
	    // elements
	    for (unsigned i = 0; i < MutatePos.size(); i += 2) {
		LockUnlockPairs::lockUnlockPair *curPair;
		curPair = lockPairs.getPair(MutatePos[i], MutatePos[i+1]);
		if (!curPair) {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") is out of bounds, skipping\n";
		    continue;
		}

		// Each element in LockDir and UnlockDir corresponds to one
		// pair of items in MutatePos
		if ((i / 2) < LockDir.size()) {
		    int shiftDir;
		    shiftDir = LockDir[i/2];
		    if (shiftDir == 1) {
			errs() << "Warning: a shift of positive 1 is a no-op\n";
		    }
		    else {
			shiftCallInst(curPair->lockCall, shiftDir);
		    }
		}
		else {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") has no lock shift direction specified\n";
		}
		if ((i / 2) < UnlockDir.size()) {
		    int shiftDir;
		    shiftDir = UnlockDir[i/2];
		    if (shiftDir == 1) {
			errs() << "Warning: a shift of positive 1 is a no-op\n";
		    }
		    else {
			modified = true;
			shiftCallInst(curPair->unlockCall, shiftDir);
		    }
		}
		else {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") has no unlock shift direction specified\n";
		}
	    } // end for
	} // end else if
	else if (splitMode) {
#ifdef MUT_DEBUG
	    errs() << "DEBUG: in split mode\n";
#endif
	    LockUnlockPairs::lockUnlockPair *curPair;
	    for (unsigned i = 0; i < MutatePos.size(); i +=2) {
		curPair = lockPairs.getPair(MutatePos[i], MutatePos[i+1]);
		if (!curPair) {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") is out of bounds, skipping\n";
		    continue;
		}
		int dist;
		int unlockPos;
		int lockPos;

		dist = lockPairs.calcDistanceBetween(curPair->lockCall, curPair->unlockCall);
#ifdef MUT_DEBUG
		errs() << "DEBUG: distance between pair == " << dist << '\n';
#endif
		if (i < SplitPos.size()) {
		    // SplitPos is guaranteed to be even so i+1 should exist
		    unlockPos = SplitPos[i];
		    lockPos = SplitPos[i+1];
		}
		else {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") has no split positions specified, "
			   << "skipping\n";
		    continue;
		}
		if (unlockPos >= dist) {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") has an unlock position that "
			   << "is greater than the distance between the pair, "
			   << "skipping\n"
			   << "\tdistance == " << dist << '\n'
			   << "\tlockPosition == " << unlockPos << '\n';
		    continue;
		}
		if (lockPos >= dist) {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") has a lock position that "
			   << "is greater than the distance between the pair, "
			   << "skipping\n"
			   << "\tdistance == " << dist << '\n'
			   << "\tlockPosition == " << lockPos << '\n';
		    continue;
		}
#ifdef MUT_DEBUG
		errs() << "DEBUG: unlockPos == " << unlockPos << '\n'
		       << "DEBUG: lockPos == " << lockPos << '\n';
#endif

		// Create copies of the lock and unlock calls
		if (curPair->lockCall->getNumArgOperands() < 1) {
		    errs() << "Warning: encountered a lockCall with less than "
			      "one argument operand, skipping\n";
		    continue;
		}
		if (curPair->unlockCall->getNumArgOperands() < 1) {
		    errs() << "Warning: encountered an unlock call with less than "
			      "one argument operand, skipping\n";
		    continue;
		}
		modified = true;
		CallInst *lockCall;
		CallInst *unlockCall;
		ArrayRef<Value *> lockArgs(curPair->lockCall->getArgOperand(0));
		ArrayRef<Value *> unlockArgs(curPair->unlockCall->getArgOperand(0));

		lockCall = CallInst::Create(curPair->lockCall->getCalledFunction(), 
			    lockArgs, "mut_lockSplit");
		unlockCall = CallInst::Create(curPair->unlockCall->getCalledFunction(), 
			    unlockArgs, "mut_unlockSplit");


		insertInstructionRelative(curPair->lockCall, unlockCall, unlockPos);
		if (unlockPos < lockPos) {
		    // Add 1 to the lock position to account for the fact that
		    // the unlock call was just inserted in its path
		    lockPos += 1;
		}
		insertInstructionRelative(curPair->lockCall, lockCall, lockPos);
	    }

	}

	return modified;
    }

    virtual void print(llvm::raw_ostream &O, const Module *M) const {
	if (!verbose) {
	    errs() << lockPairs.getFuncsSize() << "\n";
	    for (unsigned i = 0; i < lockPairs.getFuncsSize(); i++) {
		errs() << lockPairs.getPairsSizeAtFunc(i) << '\t';
	    }
	    errs() << '\n';
	}
	else {
	    lockPairs.printDebugInfo();
	}
    }

    void checkCommandLineArgs() {
	if (rmMode && swapMode) {
	    errs() << "Error: -rm and -swap cannot be specified at the same time\n";
	    exit(EXIT_FAILURE);
	}
	if (rmMode && shiftMode) {
	    errs() << "Error: -rm and -shift cannot be specified at the same time\n";
	    exit(EXIT_FAILURE);
	}
	if (rmMode && splitMode) {
	    errs() << "Error: -rm and -split cannot be specified at the same time\n";
	    exit(EXIT_FAILURE);
	}
	if (swapMode && shiftMode) {
	    errs() << "Error: -swap and -shift cannot be specified at the same time\n";
	    exit(EXIT_FAILURE);
	}
	if (swapMode && splitMode) {
	    errs() << "Error: -swap and -split cannot be specified at the same time\n";
	    exit(EXIT_FAILURE);
	}
	if (shiftMode && splitMode) {
	    errs() << "Error: -shift and -split cannot be specified at the same time\n";
	    exit(EXIT_FAILURE);
	}

	if (rmMode) {
	    if (MutatePos.size() == 0) {
		errs() << "Error: -rm but no positions to remove (see -pos)\n";
		exit(EXIT_FAILURE);
	    }
	    if (MutatePos.size() % 2) {
		errs() << "Error: -pos requires an even number of arguments with -rm(pairs)\n";
		exit(EXIT_FAILURE);
	    }
	    if (LockDir.size() != 0) {
		errs() << "Error: -lockdir is not used with -rm\n";
		exit(EXIT_FAILURE);
	    }
	    if (UnlockDir.size() != 0) {
		errs() << "Error: -unlockdir is not used with -rm\n";
		exit(EXIT_FAILURE);
	    }
	    if (SplitPos.size() != 0) {
		errs() << "Error: -splitpos is not used with -rm\n";
		exit(EXIT_FAILURE);
	    }
	}

	if (swapMode) {
	    if (MutatePos.size() == 0) {
		errs() << "Error: -swap but no positions set to swap (see -pos)\n";
		exit(EXIT_FAILURE);
	    }
	    if (MutatePos.size() % 4) {
		errs() << "Error: -swap requires -pos to be specified in groups of 4 (pairs of pairs)\n";
		exit(EXIT_FAILURE);
	    }
	    if (LockDir.size() != 0) {
		errs() << "Error: -lockdir is not used with -swap\n";
		exit(EXIT_FAILURE);
	    }
	    if (UnlockDir.size() != 0) {
		errs() << "Error: -unlockdir is not used with -swap\n";
		exit(EXIT_FAILURE);
	    }
	    if (SplitPos.size() != 0) {
		errs() << "Error: -splitpos is unused with -swap\n";
		exit(EXIT_FAILURE);
	    }
	}

	if (shiftMode) {
	    if (MutatePos.size() == 0) {
		errs() << "Error: -shift but no positions set to mutate (see -pos)\n";
		exit(EXIT_FAILURE);
	    }
	    if (MutatePos.size() % 2) {
		errs() << "Error: -pos with -shift requires an even number of arguments (pairs)\n";
		exit(EXIT_FAILURE);
	    }
	    if (LockDir.size() == 0 && UnlockDir.size() == 0) {
		errs() << "Error: -shift specified with no directions "
			  "(see -lockdir and -unlockdir)\n";
		exit(EXIT_FAILURE);
	    }
	    if (SplitPos.size() != 0) {
		errs() << "Error: -splitpos is unused with -shift\n";
		exit(EXIT_FAILURE);
	    }
	}
	
	if (splitMode) {
	    if (MutatePos.size() == 0) {
		errs() << "Error: -split but not positions set to mutate (see -pos)\n";
		exit(EXIT_FAILURE);
	    }
	    if (MutatePos.size() % 2) {
		errs() << "Error: -pos with -shift requires an even number of arguments (pairs)\n";
		exit(EXIT_FAILURE);
	    }
	    if (LockDir.size() != 0) {
		errs() << "Error: -lockdir is unused with -split\n";
		exit(EXIT_FAILURE);
	    }
	    if (UnlockDir.size() != 0) {
		errs() << "Error: -unlockdir is unused with -split\n";
		exit(EXIT_FAILURE);
	    }
	    if (SplitPos.size() == 0) {
		errs() << "Error: -splitpos is required to atleast have one pair with -shift\n";
		exit(EXIT_FAILURE);
	    }
	    if (SplitPos.size() % 2) {
		errs() << "Error: values to -splitpos need to be specified in pairs\n";
		exit(EXIT_FAILURE);
	    }
	}
    }

    void shiftCallInst(CallInst *inst, int dir) {
	if (dir == 0) {
	    return;
	}
#ifdef MUT_DEBUG
	errs() << "DEBUG: shifting instruction " << *inst << '\n';
#endif

	// Get an inst_iterator to the function
	inst_iterator iter = inst_begin(inst->getParent()->getParent());

	// Progress the iterator forward to the current instruction
	while (&*iter != inst) {
	    ++iter;
	}

#ifdef MUT_DEBUG
	errs() << "DEBUG: after moving iterator, it is: " << *iter << '\n';
#endif
	if (dir < 0) {
	    // Move the iterator backwards dir places or until the begining of
	    // the function
	    inst_iterator B = inst_begin(inst->getParent()->getParent());
	    for (int i = 0; iter != B && i != dir; --iter, --i) {
	    }
	}
	else if (dir > 0) {
	    inst_iterator E = inst_end(inst->getParent()->getParent());
	    for (int i = 0; iter != E && i != dir; ++iter, ++i) {
	    }
	}

#ifdef MUT_DEBUG
	errs() << "DEBUG: after moving iterator to shift pos, it is: " << *iter << '\n';
#endif

	// Create a copy of the CallInst
	CallInst *instCopy;
	if (inst->getNumArgOperands() < 1) {
	    errs() << "Warning: found pthread_lock CallInst with less than 1 operand "
		      "skipping\n";
	    return;
	}
	ArrayRef<Value *> args(inst->getArgOperand(0));
	instCopy = CallInst::Create(inst->getCalledFunction(), args, "mut_shift");

	int ret; 
	ret = eraseFromParentOrReplace(inst, 0, sizeof(int), true);
	if (ret) {
	    errs() << "Warning: eraseFromParentOrReplace() returned non-zero\n";
	}

	// insert the instruction before the iterator positions
	BasicBlock *bb;
	bb = iter->getParent();	// parent of an instruction is a basicblock
	bb->getInstList().insert(&*iter, instCopy);
    }

    // Inserts insertMe before the instruction distance instructions from base
    void insertInstructionRelative(Instruction *base, Instruction *insertMe, unsigned distance) {
	inst_iterator iter = inst_begin(base->getParent()->getParent());

	// Obtain iterator to the base
	while (&*iter != base) {
	    ++iter;
	}

	// Advance the iterator distance forward
	inst_iterator end = inst_end(base->getParent()->getParent());
	for (unsigned i = 0; i < distance; i++) {
	    ++iter;
	    if (iter == end) {
		errs() << "Warning: insertInstructionRelative() reached end of function "
			  "before distance was reached\n";
		break;
	    }
	}

	// Perform the insertion
	BasicBlock *bb;
	bb = iter->getParent();
	bb->getInstList().insert(&*iter, insertMe);
    }

}; // struct
} // namespace

char RmLockPair::ID = 0;
static RegisterPass<RmLockPair> X("PosixLock", "mutate pairs of calls to pthread_lock and unlock", false, false);
