/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file FindLockUnlockPairs.cpp
 * Author: Markus Kusano
 * Date: 2013-01-25
 *
 * Pass to find lock/unlock pairs
 */

#include "LockUnlockVisit.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/AliasAnalysis.h"

namespace {
    struct FindLockUnlockPairs : public ModulePass {
	static char ID;

	// Visitor to find Lock/Unlock pairs
	LockUnlockVisit pairVisit;

	FindLockUnlockPairs() : ModulePass(ID) { }

	virtual bool runOnModule(Module &M) {
	    AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
	    pairVisit.sendAliasInfo(AA);
	    pairVisit.visit(M);

	    return false;
	}

	virtual void getAnalysisUsage(AnalysisUsage &AU) const {
	    AU.addRequired<AliasAnalysis>();
	    AU.addPreserved<AliasAnalysis>();
	}
    };
}

char FindLockUnlockPairs::ID = 0;
static RegisterPass<FindLockUnlockPairs> X("FindLockUnlockPairs", "Find POSIX Lock/Unlock pairs", false, false);

