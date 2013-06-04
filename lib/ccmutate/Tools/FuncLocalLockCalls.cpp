/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 */
#include "FuncLocalLockCalls.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#define MUT_DEBUG

void FuncLocalLockCalls::search(Module &M) {
    // Obtain a function iterator to the module
    Module::iterator fIter = M.begin();
    Module::iterator fEnd = M.end();

    // Iterate over each function
    for (; fIter != fEnd; ++fIter) {
	// Create a vector to hold CallInst for this function that are calling
	// pthread_mutex_{lock,unlock}
	std::vector<CallInst *> *lockCalls;
	lockCalls = new std::vector<CallInst *>;

	// Iterate over each instruction
	inst_iterator I;
	inst_iterator E;
	for (I = inst_begin(&*fIter), E = inst_end(&*fIter);
		I != E; ++I) {

	    // Find CallInst calling pthread_mutex_{lock,unlock}
	    if (CallInst* callInst = dyn_cast<CallInst>(&*I)) {
		Function *calledFunc = callInst->getCalledFunction();
		if (calledFunc) {
		    if (calledFunc->getName() == "pthread_mutex_lock" ||
			    calledFunc->getName() == "pthread_mutex_unlock") {
			lockCalls->push_back(callInst);
		    }
		}
	    }
	}

	if (lockCalls->size() == 0) {
	    // this function has no mutex calls
	    delete lockCalls;
	}
	else {
	    funcs.push_back(&*fIter);
	    calls.push_back(lockCalls);
	}
    }
}

Function *FuncLocalLockCalls::getFuncPtr(unsigned index) const {
    if (index < funcs.size()) {
	return funcs[index];
    }

#ifdef MUT_DEBUG
    errs() << "DEBUG: index to getFuncPtr out-of-bounds\n";
#endif

    return NULL;
}

CallInst *FuncLocalLockCalls::getCallInstPtr(unsigned funcIndex, unsigned callIndex) {
    if (funcIndex < calls.size()) {
	if (callIndex < calls[funcIndex]->size()) {
	    return calls[funcIndex]->at(callIndex);
	}
#ifdef MUT_DEBUG
	else {
	    errs() << "DEBUG: callINdex to getCallInstPtr() out-of-bounds\n";
	}
#endif
    }
#ifdef MUT_DEBUG
    else {
	errs() << "DEBUG: funcIndex to getCallInstPtr() out-of-bounds\n";
    }
#endif

    return NULL;
}

unsigned FuncLocalLockCalls::getFuncsSize() const {
    return funcs.size();
}

unsigned FuncLocalLockCalls::getCallsSize() const {
    return calls.size();
}

unsigned FuncLocalLockCalls::getCallsSizeAt(unsigned index) const {
    if (index < calls.size()) {
	return calls[index]->size();
    }
    else {
	errs() << "Warning: index to getCallsSize is out-of-bounds\n";
	return 0;
    }
}


void FuncLocalLockCalls::dump() {
    for (unsigned i = 0; i < funcs.size(); i++) {
	errs() << "Function: " << funcs[i]->getName() << " has the following lock/unlock calls:\n";
	if (i < calls.size()) {
	    for (unsigned j = 0; j < calls[i]->size(); j++) {
		errs() << "\t" << *(calls[i]->at(j)) << '\n';
	    }
	}
	else {
	    errs() << "Warning: mismatch of sizes of function and callinst vectors\n";
	}
    }
}
