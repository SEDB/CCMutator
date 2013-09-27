/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file Mutex.cpp
 * \author Markus Kusano
 * \date 2013-04-01
 *
 * Positions with -pos are indexed relative to four internal data structures
 * for different combiations of lock-unlock calls being either CallInsts or
 * InvokeInsts.
 *
 * The first number specified with pos is the data structure, the second if the
 * index in that data structure:
 *  0: CallCallPairs
 *  1: CallInvokePairs
 *  2: InvokeCallPairs
 *  3: InvokeInvokePairs
 */
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "../Tools/RemoveInst.h"
#include "../Tools/ItaniumDemangle.h"

#include "llvm/Support/InstIterator.h"

#include "LockUnlockPairs.h"

// Enable debugging messages
//#define MUT_DEBUG

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
struct StdMutex : public ModulePass {
    enum LockUnlockType {
        CallCall,
        CallInvoke,
        InvokeCall,
        InvokeInvoke,
        TypeError
    };

    static char ID;

    StdMutex() : ModulePass(ID) { }

    LockUnlockPairs lockPairs;

    // Sets of instructions to mutate
    SmallPtrSet<CallInst *, 64> mutateCalls;
    SmallPtrSet<InvokeInst *, 64> mutateInvokes;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<AliasAnalysis>();
	AU.addPreserved<AliasAnalysis>();
    }

    virtual bool runOnModule(Module &M) {
	bool modified; // indicates if the code has been modified
	AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
	modified = false;

	checkCommandLineArgs();

	lockPairs.enumerate(M, AA);

	if (rmMode) {
#ifdef MUT_DEBUG
	    errs() << "DEBUG: In rmMode\n";
#endif
	    //LockUnlockPairs::lockUnlockPair *curPair;
	    for (unsigned i = 0; i < MutatePos.size(); i += 2) {
                unsigned pos1, pos2;
                pos1 = MutatePos[i];
                pos2 = MutatePos[i+1];
                if (pos1 == 0) {
                    LockUnlockPairs::CallCallLockPair *curCallCallPair;
                    curCallCallPair = lockPairs.getCallCallPair(pos2);
                    if (curCallCallPair == NULL) {
                        posOutOfBoundsWarning(pos1, pos2);
                    }
                    else {
#ifdef MUT_DEBUG
                        errs() << "DEBUG: adding to remove: " << *(curCallCallPair->lockCall) << '\n';
                        errs() << "DEBUG: adding to remove: " << *(curCallCallPair->unlockCall) << '\n';
#endif
                        mutateCalls.insert(curCallCallPair->lockCall);
                        mutateCalls.insert(curCallCallPair->unlockCall);
                    }
                }
                else if (pos1 == 1) {
                    // TODO: No test case for Call-Invoke pair found
#ifdef MUT_DEBUG
                    errs() << "DEBUG: removing CallInvoke pair (found test case)\n";
#endif
                    LockUnlockPairs::CallInvokeLockPair *curCallInvokePair;
                    curCallInvokePair = lockPairs.getCallInvokePair(pos2);
                    if (curCallInvokePair == NULL) {
                        posOutOfBoundsWarning(pos1, pos2);
                    }
                    else {
#ifdef MUT_DEBUG
                        errs() << "DEBUG: adding to remove: " << *(curCallInvokePair->lockCall) << '\n';
                        errs() << "DEBUG: adding to remove: " << *(curCallInvokePair->unlockInvoke) << '\n';
#endif
                        mutateCalls.insert(curCallInvokePair->lockCall);
                        mutateInvokes.insert(curCallInvokePair->unlockInvoke);
                    }
                }
                else if (pos1 == 2) {
                    LockUnlockPairs::InvokeCallLockPair *curInvokeCallPair;
                    curInvokeCallPair = lockPairs.getInvokeCallPair(pos2);
                    if (curInvokeCallPair == NULL) {
                        posOutOfBoundsWarning(pos1, pos2);
                    }
                    else {
#ifdef MUT_DEBUG
                        errs() << "DEBUG: adding to remove: " << *(curInvokeCallPair->lockInvoke) << '\n';
                        errs() << "DEBUG: adding to remove: " << *(curInvokeCallPair->unlockCall) << '\n';
#endif
                        mutateInvokes.insert(curInvokeCallPair->lockInvoke);
                        mutateCalls.insert(curInvokeCallPair->unlockCall);
                    }

                }
                else if (pos1 == 3){
                    // TODO: No test case for Invoke-Invoke pair found
#ifdef MUT_DEBUG
                    errs() << "DEBUG: removing InvokeInvoke pair (found test case)\n";
#endif
                    LockUnlockPairs::InvokeInvokeLockPair *curInvokeInvokePair;
                    curInvokeInvokePair = lockPairs.getInvokeInvokePair(pos2);
                    if (curInvokeInvokePair == NULL) {
                        posOutOfBoundsWarning(pos1, pos2);
                    }
                    else {
#ifdef MUT_DEBUG
                        errs() << "DEBUG: adding to remove: " << *(curInvokeInvokePair->lockInvoke) << '\n';
                        errs() << "DEBUG: adding to remove: " << *(curInvokeInvokePair->unlockInvoke) << '\n';
#endif
                        mutateInvokes.insert(curInvokeInvokePair->lockInvoke);
                        mutateInvokes.insert(curInvokeInvokePair->unlockInvoke);
                    }
                }
                else {
                    posOutOfBoundsWarning(pos1, pos2);
                }
            } // end for

            if (mutateCalls.size() > 0) {
                modified = true;
            }
            else if (mutateInvokes.size() > 0) {
                modified = true;
            } // modified implicitly set false
	    SmallPtrSet<CallInst *, 64>::iterator Ic = mutateCalls.begin();
	    SmallPtrSet<CallInst *, 64>::iterator Ec = mutateCalls.end();
	    for (;Ic != Ec; ++Ic) {
#ifdef MUT_DEBUG
                errs() << "DEBUG: erasing call\n";
#endif
                eraseLockCall(*Ic);
	    }
	    SmallPtrSet<InvokeInst *, 64>::iterator Ii = mutateInvokes.begin();
	    SmallPtrSet<InvokeInst *, 64>::iterator Ei = mutateInvokes.end();
            for (;Ii != Ei; ++Ii) {
#ifdef MUT_DEBUG
                errs() << "DEBUG: erasing invoke\n";
#endif
                eraseLockInvoke(*Ii);
            }

        }
        else if (swapMode) {
            errs() << "DEBUG: in swap mode\n";
	    for (unsigned i = 0; i < MutatePos.size(); i += 4) {
                // Pairs for each possible pair of pairs to swap. Should have
                // probably introduced some kind of generic interface in
                // LockUnlockPairs because this is getting messy.
                //LockUnlockPairs::CallCallLockPair *curCallCallPair1 = NULL;
                //LockUnlockPairs::CallInvokeLockPair *curCallInvokePair1 = NULL;
                //LockUnlockPairs::InvokeCallLockPair *curInvokeCallPair1 = NULL;
                //LockUnlockPairs::CallInvokeLockPair *curCallInvokePair1 = NULL;

                //LockUnlockPairs::CallCallLockPair *curCallCallPair2 = NULL;
                //LockUnlockPairs::CallInvokeLockPair *curCallInvokePair2 = NULL;
                //LockUnlockPairs::InvokeCallLockPair *curInvokeCallPair2 = NULL;
                //LockUnlockPairs::CallInvokeLockPair *curCallInvokePair2 = NULL;
                // Or we can just make it generic here

                void *pair1;
                void *pair2;
                LockUnlockType pair1Type;
                LockUnlockType pair2Type;

                void *lockCall1;
                void *unlockCall1;
                void *lockCall2;
                void *unlockCall2;

                bool lock1IsCall;
                bool lock2IsCall;
                bool unlock1IsCall;
                bool unlock2IsCall;

                unsigned pos1, pos2, pos3, pos4;

                // Pair 1
                pos1 = MutatePos[i];
                pos2 = MutatePos[i+1];

                // Pair 2
                pos3 = MutatePos[i+2];
                pos4 = MutatePos[i+3];
                // Indicies are checked to be valid in groups of four in
                // checkCommandLineArgs()

                pair1 = getGenericPair(pos1, pos2);
                if (pair1 == NULL) {
                    continue;
                }

                pair2 = getGenericPair(pos3, pos4);
                if (pair2 == NULL) {
                    continue;
                }

                pair1Type = getTypeFromPos(pos1);
                if (pair1Type == TypeError) {
                    errs() << "Warning: type error encountered from position 1, skipping\n";
                    continue;
                }

                pair2Type = getTypeFromPos(pos3);
                if (pair2Type == TypeError) {
                    errs() << "Warning: type error encountered from position 1, skipping\n";
                    continue;
                }

                lockCall1 = getLockCall(pair1, pair1Type, &lock1IsCall);
                if (lockCall1 == NULL) {
                    errs() << "Warning: unable to get lock from pair, skipping\n";
                    continue;
                }
                unlockCall1 = getUnlockCall(pair1, pair1Type, &unlock1IsCall);
                if (unlockCall1 == NULL) {
                    errs() << "Warning: unable to get unlock from pair, skipping\n";
                    continue;
                }
                lockCall2 = getLockCall(pair2, pair2Type, &lock2IsCall);
                if (lockCall2 == NULL) {
                    errs() << "Warning: unable to get lock from pair, skipping\n";
                    continue;
                }
                unlockCall2 = getUnlockCall(pair2, pair2Type, &unlock2IsCall);
                if (unlockCall2 == NULL) {
                    errs() << "Warning: unable to get unlock from pair, skipping\n";
                    continue;
                }

                if (lockCall1 == lockCall2) {
                    errs() << "Warning: lock pair stem from the same lock call, skipping\n";
                    continue;
                }
                if (unlockCall1 == unlockCall2) {
                    errs() << "Warning: lock pair stem from the same unlock call, skipping\n";
                    continue;
                }

                int ret;
                ret = swapCallOrInvoke(lockCall1, lockCall2, lock1IsCall, lock2IsCall);
                if (ret == 0) {
                    modified = true;
                }
                ret = swapCallOrInvoke(unlockCall1, unlockCall2, unlock1IsCall, unlock2IsCall);
                if (ret == 0) {
                    modified = true;
                }


            } // end for
        } // end else if

        else if (shiftMode) {
#ifdef MUT_DEBUG
            errs() << "DEBUG: in shift mode\n";
#endif
	    for (unsigned i = 0; i < MutatePos.size(); i += 2) {
                void *pair;
                LockUnlockType pairType; // pair and type info

                void *lockCall;
                void *unlockCall;
                bool lockIsCall;
                bool unlockIsCall; // call/invokes and type info

                unsigned pos1, pos2;

                // checkCommandLineArgs() guarantees that i+1 is valid
                pos1 = MutatePos[i];
                pos2 = MutatePos[i+1];

                pair = getGenericPair(pos1, pos2);
                pairType = getTypeFromPos(pos1);

                if (pairType == TypeError) {
                    errs() << "Warning: type error encountered from " << pos1 << ", skipping\n";
                    continue;
                }

                lockCall = getLockCall(pair, pairType, &lockIsCall);
                if (lockCall == NULL) {
                    errs() << "Warning: error getting lock call from pair, skipping\n";
                    continue;
                }
                unlockCall = getUnlockCall(pair, pairType, &unlockIsCall);
                if (unlockCall == NULL) {
                    errs() << "Warning: error getting unlock call from pair, skipping\n";
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
                        if (lockIsCall) {
                            shiftCallInst((CallInst *) lockCall, shiftDir);
                        }
                        else {
                            shiftInvokeInst((InvokeInst *) lockCall, shiftDir);
                        }
                        modified = true;
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
                        if (unlockIsCall) {
                            shiftCallInst((CallInst *) unlockCall, shiftDir);
                        }
                        else {
                            shiftInvokeInst((InvokeInst *) unlockCall, shiftDir);
                        }
			modified = true;
		    }
		}
		else {
		    errs() << "Warning: position pair (" << MutatePos[i] << ' '
			   << MutatePos[i+1] << ") has no unlock shift direction specified\n";
		}
            } // end for
        } // end else if (shift mode)
        else if (splitMode) {
#ifdef MUT_DEBUG
            errs() << "DEBUG: in split mode\n";
#endif
            // checkCommandLineArgs() guarantees that MutatePos will be valid
            // in pairs of two.
	    for (unsigned i = 0; i < MutatePos.size(); i +=2) {
                void *pair;
                void *lockCall;
                void *unlockCall;
                unsigned pos1;
                unsigned pos2;
                bool lockIsCall;
                bool unlockIsCall;
                CallInst *lockSplit;
                CallInst *unlockSplit;
                LockUnlockType pairType;

                pos1 = MutatePos[i];
                pos2 = MutatePos[i+1];
                pair = getGenericPair(pos1, pos2);

                if (pair == NULL) {
                    continue;
                }
                pairType = getTypeFromPos(pos1);

                if (pairType == TypeError) {
                    errs() << "Warning: type error encountered from " << pos1 << ", skipping\n";
                }

                lockCall = getLockCall(pair, pairType, &lockIsCall);
                if (lockCall == NULL) {
                    errs() << "Warning: error getting lock call from pair, skipping\n";
                    continue;
                }
                unlockCall = getUnlockCall(pair, pairType, &unlockIsCall);
                if (unlockCall == NULL) {
                    errs() << "Warning: error getting unlock call from pair, skipping\n";
                    continue;
                }

		int dist;
		int unlockPos;
		int lockPos;

		dist = lockPairs.calcDistanceBetween((Instruction *)lockCall, (Instruction *)unlockCall);
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
		// Create copies of the lock and unlock calls. Regardless of if
                // either part of the pair is an Invoke, the copies will be
                // calls since we do not intend to create/termiante basic
                // blocks during the split
                if (lockIsCall) {
                    int error;
                    lockSplit = createMutexCopy((CallInst *) lockCall, error);
                    if (error != 0) {
                        continue;
                    }
                }
                else {
                    int error;
                    lockSplit = createMutexCopy((InvokeInst *) lockCall, error);
                    if (error != 0) {
                        continue;
                    }
                }
                if (unlockIsCall) {
                    int error;
                    unlockSplit = createMutexCopy((CallInst *) unlockCall, error);
                    if (error != 0) {
                        continue;
                    }
                }
                else {
                    int error;
                    unlockSplit = createMutexCopy((InvokeInst *) unlockCall, error);
                    if (error != 0) {
                        continue;
                    }
                }

                modified = true;

		insertInstructionRelative((Instruction *)lockCall, (Instruction *)unlockSplit, unlockPos);
		if (unlockPos < lockPos) {
                    // Add 1 to the lock position to account for the fact that
                    // the unlock call was just inserted in its path
		    lockPos += 1;
		}
		insertInstructionRelative((Instruction *)lockCall, (Instruction *)lockSplit, lockPos);
            } // end for
        } // end else if splitMode
	return modified;
    }

    virtual void print(llvm::raw_ostream &O, const Module *M) const {
	if (!verbose) {
            if (lockPairs.getNumCallCallPairs() != 0)
                errs() << 0 << '\t' << lockPairs.getNumCallCallPairs() << '\n';
            if (lockPairs.getNumCallInvokePairs() != 0)
                errs() << 1 << '\t' << lockPairs.getNumCallInvokePairs() << '\n';
            if (lockPairs.getNumInvokeCallPairs() != 0)
                errs() << 2 << '\t' << lockPairs.getNumInvokeCallPairs() << '\n';
            if (lockPairs.getNumInvokeInvokePairs() != 0)
                errs() << 3 << '\t' << lockPairs.getNumInvokeInvokePairs() << '\n';
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

    void eraseLockCall(CallInst* call) {
        std::string funcName;
        Function *calledFunc;
        calledFunc = call->getCalledFunction();

        if (call) {
            funcName = getFunctionName(calledFunc);
        }
        else {
            errs() << "Warning: indirect function call found to be removed, skipping";
            return;
        }
        if (funcName == "pthread_mutex_lock" || funcName == "pthread_mutex_unlock") {
	    eraseFromParentOrReplace(call, 0, sizeof(int), true); // pthread funcs have ret val
        }
        else {
            if (!call->hasNUses(0)) {
                errs() << "Warning: found std::mutex call that has more than 0 uses, skipping\n";
            }
            else {
	        eraseFromParentOrReplace(call, 0, sizeof(int), true);
            }
        }
    }

    void *getLockCall(void *pair, LockUnlockType type, bool *isCall = NULL) {
        if (type == CallCall) {
            if (isCall != NULL)
                *isCall = true;
            return ((LockUnlockPairs::CallCallLockPair *) pair)->lockCall;
        }
        else if (type == CallInvoke) {
            if (isCall != NULL)
                *isCall = true;
            return ((LockUnlockPairs::CallInvokeLockPair *) pair)->lockCall;
        }
        else if (type == InvokeCall) {
            if (isCall != NULL)
                *isCall = false;
            return ((LockUnlockPairs::InvokeCallLockPair *) pair)->lockInvoke;
        }
        else if (type == InvokeInvoke) {
            if (isCall != NULL)
                *isCall = false;
            return ((LockUnlockPairs::InvokeInvokeLockPair *) pair)->lockInvoke;
        }
        return NULL;
    }

    void *getUnlockCall(void *pair, LockUnlockType type, bool *isCall) {
        if (type == CallCall) {
            if (isCall != NULL)
                *isCall = true;
            return ((LockUnlockPairs::CallCallLockPair *) pair)->unlockCall;
        }
        else if (type == CallInvoke) {
            if (isCall != NULL)
                *isCall = false;
            return ((LockUnlockPairs::CallInvokeLockPair *) pair)->unlockInvoke;
        }
        else if (type == InvokeCall) {
            if (isCall != NULL)
                *isCall = true;
            return ((LockUnlockPairs::InvokeCallLockPair *) pair)->unlockCall;
        }
        else if (type == InvokeInvoke) {
            if (isCall != NULL)
                *isCall = false;
            return ((LockUnlockPairs::InvokeInvokeLockPair *) pair)->unlockInvoke;
        }
        return NULL;
    }

    // Swaps gen1 with gen2 using gen{1,2}IsCall to determine if call1 or
    // call2 is a CallInst or an InvokeInst
    int swapCallOrInvoke(void *gen1, void *gen2, bool gen1IsCall, bool gen2IsCall) {
        if (gen1IsCall && gen2IsCall) {
            return swapCalls((CallInst *) gen1, (CallInst *) gen2);
        }
        else if (gen1IsCall && !gen2IsCall) {
            return swapCallInvoke((CallInst *) gen1, (InvokeInst *) gen2);
        }
        else if (!gen1IsCall && gen2IsCall) {
            return swapCallInvoke((CallInst *) gen2, (InvokeInst *) gen1);
        }
        else { // implicit both false
            return swapInvokes((InvokeInst *) gen1, (InvokeInst *) gen2);
        }
    }

    LockUnlockType getTypeFromPos(unsigned pos) {
        if (pos == 0)
            return CallCall;
        else if (pos == 1)
            return CallInvoke;
        else if (pos == 2)
            return InvokeCall;
        else if (pos == 3)
            return InvokeInvoke;

        return TypeError;
    }

    int swapCalls(CallInst *call1, CallInst *call2) {
#ifdef MUT_DEBUG
        errs() << "DEBUG: swapping:\n"
               << '\t' << *call1 << '\n'
               << "with\n"
               << '\t' << *call2 << '\n';
#endif
        Function *func1;
        Function *func2;

        bool call1IsPthread;
        bool call2IsPthread;

        func1 = call1->getCalledFunction();
        func2 = call2->getCalledFunction();

        if (func1 == NULL || func2 == NULL) {
            errs() << "Warning: indirect function call encountered, skipping\n";
            return -1;
        }

        call1IsPthread = isPthreadCall(func1);
        call2IsPthread = isPthreadCall(func2);

        if (call1IsPthread != call2IsPthread) {
            pthreadCpp11Warning();
            return -1;
        }
        // Posix and C++11 constructs are not supported to be swapped due to
        // differing return values (int and void respectively)

        // Create a copy of the instructions
        CallInst *call1Copy;
        CallInst *call2Copy;

        if (call1->getNumArgOperands() != 1) {
            errs() << "Warning: found lock/unlock CallInst that does not have 1 operand, skipping";
            errs() << '\t' << *call1 << '\n';
            return -1;
        }
        if (call2->getNumArgOperands() != 1) {
            errs() << "Warning: found lock/unlock CallInst that does not have 1 operand, skipping";
            errs() << '\t' << *call2 << '\n';
            return -1;
        }

        ArrayRef<Value *> call1Args(call1->getArgOperand(0));
        ArrayRef<Value *> call2Args(call2->getArgOperand(0));

        if (call1IsPthread) {
            call1Copy = CallInst::Create(func1, call1Args, "mut_call1");
            call2Copy = CallInst::Create(func2, call2Args, "mut_call2");
        }
        else { // void return ==> no name
            call1Copy = CallInst::Create(func1, call1Args);
            call2Copy = CallInst::Create(func2, call2Args);
        }

        // Copy over attributes
        call1Copy->setAttributes(call1->getAttributes());
        call2Copy->setAttributes(call2->getAttributes());


#ifdef MUT_DEBUG
        errs() << "DEBUG: Performing replacment. Replacing:\n" << *call1 << "\nwith\n"
               << *call2Copy << '\n';
#endif

        BasicBlock::iterator iter1(call1);
        ReplaceInstWithInst(call1->getParent()->getInstList(),
                iter1, call2Copy);

#ifdef MUT_DEBUG
        errs() << "DEBUG: Performing replacment. Replacing:\n" << *call2 << "\nwith\n"
               << *call1Copy << '\n';
#endif
        BasicBlock::iterator iter2(call2);
        ReplaceInstWithInst(call2->getParent()->getInstList(),
                iter2, call1Copy);

        return 0;
    }

    int swapCallInvoke(CallInst *call1, InvokeInst *invoke2) {
#ifdef MUT_DEBUG
        errs() << "Swapping:\n\t" << *call1 << "\n\t" << *invoke2 << '\n';
#endif

        errs() << "Warning: this function has not been tested on, runtime "
                  "errors/malformed LLVM may result\nPlease let me(markus) "
                  "know about this if you run into problems\n\tfunction: "
                  "StdMutex::swapCallInvoke\n";

        Function *callFunc;
        Function *invokeFunc;

        callFunc = call1->getCalledFunction();
        invokeFunc = invoke2->getCalledFunction();

        bool call1IsPthread;
        bool invoke2IsPthread;

        if (callFunc == NULL || invokeFunc == NULL) {
            errs() << "Warning: indirect function call encountered, skipping\n";
            return -1;
        }

        call1IsPthread = isPthreadCall(callFunc);
        invoke2IsPthread = isPthreadCall(invokeFunc);

        if (call1IsPthread != invoke2IsPthread) {
            pthreadCpp11Warning();
            return -1;
        }

        if (call1->getNumArgOperands() != 1) {
            errs() << "Warning: found lock/unlock CallInst that does not have 1 operand, skipping";
            errs() << '\t' << *call1 << '\n';
            return -1;
        }
        if (invoke2->getNumArgOperands() != 1) {
            errs() << "Warning: found lock/unlock InvokeInst that does not have 1 operand, skipping";
            errs() << '\t' << *invoke2 << '\n';
            return -1;
        }

        // Create copies of the CallInst/InvokeInst but swap the called
        // functions and operands.
        CallInst *callWithInvokeFunc;
        InvokeInst *invokeWithCallFunc;

        BasicBlock *ifNormal;
        BasicBlock *ifException;

        ifNormal = invoke2->getNormalDest();
        ifException = invoke2->getUnwindDest();

        ArrayRef<Value *> call1Args(call1->getArgOperand(0));
        ArrayRef<Value *> invoke2Args(invoke2->getArgOperand(0));


        if (call1IsPthread) {
            callWithInvokeFunc = CallInst::Create(invokeFunc, invoke2Args, "mut_callInvoke");
            invokeWithCallFunc = InvokeInst::Create(callFunc, ifNormal, ifException, 
                    call1Args, "mut_invokeCall");
        }
        else { // void return ==> no name
            callWithInvokeFunc = CallInst::Create(invokeFunc, invoke2Args);
            invokeWithCallFunc = InvokeInst::Create(callFunc, ifNormal, ifException, 
                    call1Args);
        }

        // Copy Attributes
        callWithInvokeFunc->setAttributes(invoke2->getAttributes());
        invokeWithCallFunc->setAttributes(call1->getAttributes());

        BasicBlock::iterator iter1(call1);
        ReplaceInstWithInst(call1->getParent()->getInstList(),
                iter1, callWithInvokeFunc);

        BasicBlock::iterator iter2(invoke2);
        ReplaceInstWithInst(invoke2->getParent()->getInstList(),
                iter2, invokeWithCallFunc);

        return 0;
    }

    // Swaps the called function of two invoke instructions. Their normal
    // destination and unwind destination are unchanged. They are assumed to be
    // either POSIX or C++11 mutex lock/unlock invokes
    int swapInvokes(InvokeInst *invoke1, InvokeInst *invoke2) {
#ifdef MUT_DEBUG
        errs() << "DEBUG: Swapping:\n\t" << *invoke1 << "\n\t" << *invoke2 << '\n';
#endif

        Function *invoke1Func;
        Function *invoke2Func;

        invoke1Func = invoke1->getCalledFunction();
        invoke2Func = invoke2->getCalledFunction();

        bool invoke1IsPthread;
        bool invoke2IsPthread;

        if (invoke1Func == NULL || invoke2Func == NULL) {
            errs() << "Warning: indirect function call encountered, skipping\n";
            return -1;
        }

        invoke1IsPthread = isPthreadCall(invoke1Func);
        invoke2IsPthread = isPthreadCall(invoke2Func);

        if (invoke1IsPthread != invoke2IsPthread) {
            pthreadCpp11Warning();
            return -1;
        }

        if (invoke1->getNumArgOperands() != 1) {
            errs() << "Warning: found lock/unlock InvokeInst that does not have 1 operand, skipping";
            errs() << '\t' << *invoke1 << '\n';
            return -1;
        }
        if (invoke2->getNumArgOperands() != 1) {
            errs() << "Warning: found lock/unlock InvokeInst that does not have 1 operand, skipping";
            errs() << '\t' << *invoke2 << '\n';
            return -1;
        }

        // Create copies of the InvokeInsts but swap the called functions and
        // operands.
        InvokeInst *invoke1Func2;
        InvokeInst *invoke2Func1;

        BasicBlock *ifNormal1;
        BasicBlock *ifException1;
        BasicBlock *ifNormal2;
        BasicBlock *ifException2;

        ifNormal1 = invoke1->getNormalDest();
        ifException1 = invoke1->getUnwindDest();
        ifNormal2 = invoke2->getNormalDest();
        ifException2 = invoke2->getUnwindDest();

        ArrayRef<Value *> invoke1Args(invoke1->getArgOperand(0));
        ArrayRef<Value *> invoke2Args(invoke2->getArgOperand(0));


        if (invoke1IsPthread) {
            invoke1Func2 = InvokeInst::Create(invoke2Func, ifNormal1, ifException1, 
                    invoke2Args, "mut_invokeCall");
            invoke2Func1 = InvokeInst::Create(invoke1Func, ifNormal2, ifException2, 
                    invoke1Args, "mut_invokeCall");
        }
        else { // void return ==> no name
            invoke1Func2 = InvokeInst::Create(invoke2Func, ifNormal1, ifException1, 
                    invoke2Args);
            invoke2Func1 = InvokeInst::Create(invoke1Func, ifNormal2, ifException2, 
                    invoke1Args);
        }

        // Copy Attributes
        invoke1Func2->setAttributes(invoke2->getAttributes());
        invoke2Func1->setAttributes(invoke1->getAttributes());

        BasicBlock::iterator iter1(invoke1);
        ReplaceInstWithInst(invoke1->getParent()->getInstList(),
                iter1, invoke1Func2);

        BasicBlock::iterator iter2(invoke2);
        ReplaceInstWithInst(invoke2->getParent()->getInstList(),
                iter2, invoke2Func1);

        return 0;
    }

    bool isPthreadCall(Function *F) {
        if (F != NULL) {
            if (F->getName() == "pthread_mutex_lock" || F->getName() == "pthread_mutex_unlock") {
                return true;
            }
        }
        return false;
    }
    void eraseLockInvoke(InvokeInst* call) {
        std::string funcName;
        Function *calledFunc;
        calledFunc = call->getCalledFunction();

        if (call) {
            funcName = getFunctionName(calledFunc);
        }
        else {
            errs() << "Warning: indirect function call found to be removed, skipping";
            return;
        }
        if (funcName == "pthread_mutex_lock" || funcName == "pthread_mutex_unlock") {
	    eraseInvokeOrRep(call, 0, sizeof(int), true); // pthread funcs have ret val
        }
        else {
            if (!call->hasNUses(0)) {
                errs() << "Warning: found std::mutex call that has more than 0 uses, skipping\n";
            }
            else {
	        eraseInvokeOrRep(call, 0, sizeof(int), true); // pthread funcs have ret val
            }
        }
    }

    void shiftCallInst(CallInst *inst, int dir) {
        bool isPthread;
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

        isPthread = isPthreadCall(inst->getCalledFunction());

	// Create a copy of the CallInst
	CallInst *instCopy;
	if (inst->getNumArgOperands() < 1) {
	    errs() << "Warning: found pthread_lock CallInst with less than 1 operand "
		      "skipping\n";
	    return;
	}
	ArrayRef<Value *> args(inst->getArgOperand(0));
        if (isPthread) {
            instCopy = CallInst::Create(inst->getCalledFunction(), args, "mut_shift");
        }
        else {
            if (!inst->hasNUses(0)) {
                errs() << "Warning: found std::mutex call with more than 0 uses, skipping\n";
                return;
            }
            instCopy = CallInst::Create(inst->getCalledFunction(), args); // no return ==> no name
        }

        // Copy Attributes
        instCopy->setAttributes(inst->getAttributes());

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

    // Shifts an invoke instruction. If the invoke has uses, it is replaced
    // with a constant int of 0. Then, a branch instruction is inserted after
    // the constant 0 (if the invoke has no uses then it is simply replaced
    // with a branch). The invoke is then replaced with a call and shifted.
    void shiftInvokeInst(InvokeInst *inst, int dir) {
        BranchInst *normalBranch;
        BasicBlock *ifNormal;
        CallInst *callCopy;
        bool isPthread;

#ifdef MUT_DEBUG
        errs() << "DEBUG: shifting invoke: " << *inst << '\n';
#endif

        isPthread = isPthreadCall(inst->getCalledFunction());
        if (!isPthread) {
            if (!inst->hasNUses(0)) {
                errs() << "Warning: found std::mutex call with more than 0 uses, skipping\n";
                return;
            }
        }

        if (inst->getNumArgOperands() != 1) {
            errs () << "Warning, found mutex call with more than one operand, skipping\n";
            return;
        }

        ifNormal = inst->getNormalDest();
        normalBranch = BranchInst::Create(ifNormal, inst->getParent());
        (void) normalBranch; // remove set but not used warning
        // normalBranch is now the basicBlock terminator.

        // Create a CallInst version of the invoke and insert it before the
        // current InvokeInst
	ArrayRef<Value *> args(inst->getArgOperand(0));
        if (isPthread) {
            callCopy = CallInst::Create(inst->getCalledFunction(), args, "mut_shift", inst);
        }
        else {
            callCopy = CallInst::Create(inst->getCalledFunction(), args, "", inst);
        }

        // Copy Attributes
        callCopy->setAttributes(inst->getAttributes());

        // Erase the instruction. It wont be replaced because it has 0 uses.
        int ret;
        ret = eraseFromParentOrReplace(inst, 0, sizeof(int), true);
        if (ret) {
            errs() << "Warning: eraseFromParentOrReplace() returned non-zero\n";
        }

        // Perform the shift
        shiftCallInst(callCopy, dir);
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

    void posOutOfBoundsWarning(int pos1, int pos2) {
        errs() << "Warning: position (" << pos1 << ", " << pos2
               << ") is out of bounds, skipping\n";
    }

    void pthreadCpp11Warning() {
        errs() << "Warning: unable to mutate pairs of pairs involving POSIX and C++11, skipping\n";
    }

    // Returns a pointer to the LockUnlock pair at pos1 and pos2. pos1
    // determinest the type so the caller should know this. Returns NULL on
    // failure and will output a warning message.
    void *getGenericPair(unsigned pos1, unsigned pos2) {
        void *pair = NULL;

        if (pos1 == 0) { // CallCall
            pair = lockPairs.getCallCallPair(pos2);
            if (pair == NULL) {
                posOutOfBoundsWarning(pos1, pos2);
            }
#ifdef MUT_DEBUG
            else {
                errs() << "DEBUG: Found pair:\n\t" 
                       << *(((LockUnlockPairs::CallCallLockPair *) pair)->lockCall) << "\n\t"
                       << *(((LockUnlockPairs::CallCallLockPair *) pair)->unlockCall) << "\n";
            }
#endif
        }
        else if (pos1 == 1) { // CallInvoke
            // TODO: This needs a test case
#ifdef MUT_DEBUG
            errs() << "DEBUG: call-invoke test found\n";
#endif
            pair = lockPairs.getCallInvokePair(pos2);
            if (pair == NULL) {
                posOutOfBoundsWarning(pos1, pos2);
            }
#ifdef MUT_DEBUG
            else {
                errs() << "DEBUG: Found pair:\n\t" 
                       << *(((LockUnlockPairs::CallInvokeLockPair *) pair)->lockCall) << "\n\t"
                       << *(((LockUnlockPairs::CallInvokeLockPair *) pair)->unlockInvoke) << "\n";
            }
#endif
        }
        else if (pos1 == 2) { // InvokeCall
            pair = lockPairs.getInvokeCallPair(pos2);
            if (pair == NULL) {
                posOutOfBoundsWarning(pos1, pos2);
            }
#ifdef MUT_DEBUG
            else {
                errs() << "DEBUG: Found pair:\n\t" 
                       << *(((LockUnlockPairs::InvokeCallLockPair *) pair)->lockInvoke) << "\n\t"
                       << *(((LockUnlockPairs::InvokeCallLockPair *) pair)->unlockCall) << "\n";
            }
#endif
        }
        else if (pos1 == 3) { // InvokeInvoke
            // TODO: this needs a test case
#ifdef MUT_DEBUG
            errs() << "DEBUG: invoke-invoke test found\n";
#endif
            pair = lockPairs.getInvokeInvokePair(pos1);
            if (pair == NULL) {
                posOutOfBoundsWarning(pos1, pos2);
            }
#ifdef MUT_DEBUG
            else {
                errs() << "DEBUG: Found pair:\n\t" 
                       << *(((LockUnlockPairs::InvokeInvokeLockPair *) pair)->lockInvoke) << "\n\t"
                       << *(((LockUnlockPairs::InvokeInvokeLockPair *) pair)->unlockInvoke) << "\n";
            }
#endif
        }
        else {
            posOutOfBoundsWarning(pos1, pos2);
            pair = NULL;
        }

        return pair;
    }

    int calcDistanceBetween(Instruction *inst1, Instruction *inst2) {
        // Check that the instructions are from the same function
        if (inst1->getParent()->getParent() != inst2->getParent()->getParent()) {
            errs() << "Warning: unable to calculate the distance between two "
                      "instructions that are in different functions";
            return -1;
        }
        int distance;
        distance = 0;

        inst_iterator iter = inst_begin(inst1->getParent()->getParent());

        // Find the first instruction in the function
        while (&*iter != inst1 && &*iter != inst2) {
            ++iter;
        }

        // Increment the iterator past the just found instruction
        ++iter;
        ++distance;
        while (&*iter != inst1 && &*iter != inst2) {
            ++distance;
            ++iter;
        }
        return distance;
    }

    CallInst *createMutexCopy(CallInst *copyMe, int &error) {
        bool isPthread;
        CallInst *ret;
        isPthread = isPthreadCall(copyMe->getCalledFunction());

        if (copyMe->getNumArgOperands() != 1) {
            errs() << "Warning: found mutex call w/o one operand, skipping\n";
            error = -1;
        }

        ArrayRef<Value *> args = copyMe->getArgOperand(0);
        if (isPthread) {
            ret = CallInst::Create(copyMe->getCalledFunction(), args, "mut_lockSplit");
        }
        else {
            ret = CallInst::Create(copyMe->getCalledFunction(), args);
        }

        ret->setAttributes(copyMe->getAttributes());
        error = 0;
        return ret;
    }

    CallInst *createMutexCopy(InvokeInst *copyMe, int &error) {
        bool isPthread;
        CallInst *ret;
        isPthread = isPthreadCall(copyMe->getCalledFunction());

        if (copyMe->getNumArgOperands() != 1) {
            errs() << "Warning: found mutex call w/o one operand, skipping\n";
            error = -1;
        }

        ArrayRef<Value *> args = copyMe->getArgOperand(0);
        if (isPthread) {
            ret = CallInst::Create(copyMe->getCalledFunction(), args, "mut_lockSplit");
        }
        else {
            ret = CallInst::Create(copyMe->getCalledFunction(), args);
        }

        ret->setAttributes(copyMe->getAttributes());
        error = 0;
        return ret;
    }
}; // struct
} // namespace

char StdMutex::ID = 0;
static RegisterPass<StdMutex> X("Mutex", "mutate pairs of calls to std::mutex::lock and std::mutex::unlock", false, false);
