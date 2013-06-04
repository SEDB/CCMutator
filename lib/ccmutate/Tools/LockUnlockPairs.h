/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file LockUnlockPairs.h
 * \author Markus Kusano
 *
 * Finds pairs of calls pthread_mutex_lock and pthread_mutex_unlock. This class
 * has an internal instance of FuncLocalLockCalls that enumerates all
 * occurences of pthread lock and pthread unlock in each function. This class
 * then pairs up lock and unlock calls that alias to the same mutex.
 *
 * This depends on the alias analysis information provided; currently only
 * function local alias information is used thus only function local
 * lock-unlock pairs are found.
 */
#include "FuncLocalLockCalls.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Module.h"

#include <vector>

using namespace llvm;

class LockUnlockPairs {
    public:
	struct lockUnlockPair {
	    CallInst *lockCall;
	    CallInst *unlockCall;
	};

	/// Find the lock and unlock calls in the passed Module using the
	/// passed AliasAnalysis results
	void visit(Module &M, AliasAnalysis &AA);

	/// Sends pair information to stderr
	void dump() const;

	/// Returns the number of functions found to have calls to pthread lock
	/// or unlock
	unsigned getFuncsSize() const;

	/// Returns the function at the given index, NULL if it is
	/// out-of-bounds
	Function *getFunc(unsigned index) const;

	/// Returns the pair at the given index, NULL if either index is
	/// out-of-bounds
	lockUnlockPair *getPair(unsigned funcIndex, unsigned pairIndex) const;

	/// Returns the number of pairs for the function at the passed index.
	/// Returns 0 and outputs a warning if the index is out-of-bounds
	unsigned getPairsSizeAtFunc(unsigned index) const;

	/// Prints debug information about the functions and lock unlock pairs
	/// to stderr.
	void printDebugInfo() const;

	/// Uses the passed AliasAnalysis information to see if the two passed
	/// CallInst alias to the same mutex
	static bool checkMutexAlias(CallInst *call1, CallInst* call2, AliasAnalysis &AA);

	/// Returns true if the passed CallInst is a call to
	/// pthread_mutex_lock, otherwise false
	static bool isCallToPThreadMutexLock(CallInst *call);

	/// Returns true if the passed CallInst is a call to
	/// pthread_mutex_unlock, otherwise false
	static bool isCallToPThreadMutexUnlock(CallInst *call);

	/// Calculates the distance between inst1 and inst2. They are required
	/// to be in the same function. Returns -1 on failure, otherwise the
	/// positive distance between the two instructions.
	static int calcDistanceBetween(Instruction *inst1, Instruction *inst2);

    private:
	FuncLocalLockCalls calls;

	/// Vector of vectors of lockUnlockPairs. Each internal vector
	/// represents the pairs for a certain function that uses pthread lock
	/// and unlock
	std::vector<std::vector<lockUnlockPair *>*> funcPairs;
};
