/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file LockUnlockVisit.cpp
 * Author: Markus Kusano
 * Date: 2013-01-15
 *
 * See LockUnlockVisit.h for more information
 */

#include "LockUnlockVisit.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"

void LockUnlockVisit::visitCallInst(CallInst &I) {
    // Check if the function being called is pthread_mutex_lock
    Function *call;
    call = I.getCalledFunction();

    if (call) {
	// Assumption: calls to pthread_mutex_lock will never be indirect and
	// their first argument is the mutex they operate on
	if (isPthreadLock(call)) {
	    DEBUG(errs() << "DEBUG: pthread_mutex_lock call found\n");
	    DEBUG(errs() << "with first argument" << call->getArgumentList().front());
	    DEBUG(errs() << '\n');
	}

	else if (isPthreadUnlock(call)) {
	    DEBUG(errs() << "DEBUG: pthread_mutex_unlock call found\n");
	    DEBUG(errs() << "with first argument" << call->getArgumentList().front());
	    DEBUG(errs() << '\n');
	}

	// DEBUG: Print out the calls argument list
	/*
	errs() << "With arguments:\n";
	Function::ArgumentListType::iterator i;
	Function::ArgumentListType::iterator e;
	for (i = call->getArgumentList().begin(), e = call->getArgumentList().end();
		i != e; i++) {
	    errs() << *i << '\n';
	}
	*/
    }
}

bool LockUnlockVisit::isPthreadLock(const Function *F) const {
    if (F->getName() == "pthread_mutex_lock") {
	return true;
    }
    return false;
}

bool LockUnlockVisit::isPthreadUnlock(const Function *F) const {
    if (F->getName() == "pthread_mutex_unlock") {
	return true;
    }
    return false;
}

void LockUnlockVisit::sendAliasInfo(AliasAnalysis &AA) {
    aliasInfo = &AA;
}
