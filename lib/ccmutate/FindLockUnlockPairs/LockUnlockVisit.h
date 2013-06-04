/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file LockUnlockVisit.h
 * Author: Markus Kusano
 * Date: 2013-01-15
 *
 * Visitor for unlock/lock function calls.
 *
 * This visitor is assumed (hopefully correctly) that it will visit each
 * function in order, and then visit each basic block of each function in order
 * and then visit each instruction of each basic block in order. I looked at
 * the InstVisitor class and I believe that this is a valid assumption.
 *
 * This file describes the implementation of most of the algorithm to find
 * lock/unlock pairs described in 'Practical Lock/Unlock Pairing for Concurrent
 * Programs. Hyoun Kyu Cho et. al. Feb 2013'. The algorithm is outlined in
 * Figure 4 of the paper. The main part of the algorithm is:
 *
 * for (each instruction s in bb in order) {
 *  if (s is lock) 
 *    push s to lock stack;
 *  else if (s in an unlock)
 *    top = top element of lock stack
 *    add s in GEN[top]
 *    for (each element e in lock stack, e != top)
 *      add s in KILL[e]
 *    pop lock stack
 * }
 *
 * where bb is a basic block of a function.
 *
 * This is then repeated in order for each bb. As noted above, it is hoped that
 * the order of visiting instructions in basic blocks will be handled by the
 * InstVisitor class. 
 */

#include <set>
#include "llvm/Support/InstVisitor.h"
#include "llvm/Analysis/AliasAnalysis.h"

using namespace llvm;

class LockUnlockVisit: public InstVisitor<LockUnlockVisit> {
    public:

	/// Set used to check if a lock has been seen before
	std::set<Value *> lockSet;

	/// Alias analysis information required by the class
	AliasAnalysis *aliasInfo;

	/// Default constructor.
	LockUnlockVisit() { aliasInfo = NULL; }

	/// Overridden visitor function for call instructions
	void visitCallInst(CallInst &I);

	/// Checks if the passed function is a call to pthread_mutex_lock
	bool isPthreadLock(const Function *F) const;

	/// Checks if the passed function is a call to pthread_mutex_unlock
	bool isPthreadUnlock(const Function *F) const;

	/// Gives this class access to required alias information
	void sendAliasInfo(AliasAnalysis &AA);
};
