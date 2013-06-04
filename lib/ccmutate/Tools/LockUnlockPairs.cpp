/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file LockUnlockPairs.cpp
 * \author Markus Kusano
 */
#include "AliasResultToString.h"
#include "LockUnlockPairs.h"
#include "llvm/DebugInfo.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#define MUT_DEBUG

// Enable even more debugging output
//#define MUT_DEBUG_VERB

void LockUnlockPairs::visit(Module &M, AliasAnalysis &AA) {
    // Enumerate all occurences
    calls.search(M);

    // For each function that has lock and unlock calls compare each lock call
    // to every subsequent unlock call, if they alias to the same mutex then
    // add them to the set of pairs
    for (unsigned i = 0; i < calls.getCallsSize(); i++) {
	// Holds pairs for the current function
	std::vector<lockUnlockPair *> *pairs = new std::vector<lockUnlockPair *>;
	for (unsigned j = 0; j < calls.getCallsSizeAt(i) - 1; j++) {
	    CallInst *inst1;
	    inst1 = calls.getCallInstPtr(i,j);
	    if (inst1 == NULL) {
		errs() << "Warning, in LockUnlockPairs, CallInst1 is NULL, skipping\n";
		continue;
	    }
	    if (!isCallToPThreadMutexLock(inst1)) {
		// We only compare lock calls to unlock calls
		continue;
	    }
	    for (unsigned k = j + 1; k < calls.getCallsSizeAt(i); k++) {
		CallInst *inst2;

		inst2 = calls.getCallInstPtr(i, k);

		if (inst2 == NULL) {
		    errs() << "Warning, in LockUnlockPairs, CallInst2 is NULL, skipping\n";
		    continue;
		}
		if (!isCallToPThreadMutexUnlock(inst2)) {
		    continue;
		}
		if (checkMutexAlias(inst1, inst2, AA)) {
		    // Found a lock unlock pair
		    lockUnlockPair *newPair = new lockUnlockPair;
		    newPair->lockCall = inst1;
		    newPair->unlockCall = inst2;
		    pairs->push_back(newPair);
		}
	    }
	}

	// Add all the found pairs to the internal data structure. There is a
	// chance that no pairs were found, meaning the vector is of size 0.
	// This is desired to show that a function has calls to pthread lock or
	// unlock but none alias to each other.
	funcPairs.push_back(pairs);
    }
}

bool LockUnlockPairs::checkMutexAlias(CallInst *call1, CallInst *call2, AliasAnalysis &AA) {
    Value *mutex1;
    Value *mutex2;

    if (call1->getNumArgOperands() < 1) {
	errs() << "Warning: found pthread_mutex_lock or unlock call with less than 1 operand\n";
	return false;
    }
    if (call2->getNumArgOperands() < 1) {
	errs() << "Warning: found pthread_mutex_lock or unlock call with less than 1 operand\n";
	return false;
    }

    mutex1 = call1->getArgOperand(0);
    mutex2 = call2->getArgOperand(0);

    AliasAnalysis::AliasResult res;

    res = AA.alias(mutex1, mutex2);

#ifdef MUT_DEBUG_VERB
    errs() << "DEBUG: Alias result of " << *call1 << " and " << *call2 << "\n"
	   << "\t" << AAResToString(res) << "\n";
#endif

    if (res == AliasAnalysis::MustAlias) {
	return true;
    }

    return false;
}

bool LockUnlockPairs::isCallToPThreadMutexLock(CallInst *call) {
    Function *F;
    F = call->getCalledFunction();

    if (!F) {
	errs() << "Warning: indirect function call passed to "
		  "LockUnlockPairs::isCallToPThreadMutexLock\n";
	return false;
    }

    if (F->getName() == "pthread_mutex_lock") {
	return true;
    }

    return false;
}

bool LockUnlockPairs::isCallToPThreadMutexUnlock(CallInst *call) {
    Function *F;
    F = call->getCalledFunction();

    if (!F) {
	errs() << "Warning: indirect function call passed to "
		  "LockUnlockPairs::isCallToPThreadMutexUnlock\n";
	return false;
    }

    if (F->getName() == "pthread_mutex_unlock") {
	return true;
    }

    return false;
}

void LockUnlockPairs::dump() const {
    for (unsigned i = 0; i < funcPairs.size(); i++) {
	errs() << "In function " << i << ":\n";
	errs() << "The following CallInsts alias to the same mutex:\n";
	for (unsigned j = 0; j < funcPairs.at(i)->size(); j++) {
	    lockUnlockPair *pair;
	    pair = funcPairs.at(i)->at(j);
	    errs() << '\t' << *(pair->lockCall) << '\n' 
		   << '\t' << *(pair->unlockCall) << '\n';
	}
    }
}

unsigned LockUnlockPairs::getFuncsSize() const {
    return funcPairs.size();
}

unsigned LockUnlockPairs::getPairsSizeAtFunc(unsigned index) const {
    if (index < funcPairs.size()) {
	return funcPairs.at(index)->size();
    }
    else {
	errs() << "Warning: in LockUnlockPairs::getPairsSizeAtFunc(), passed "
		  "index is out-of-bounds\n";
    }
    return 0;
}

Function *LockUnlockPairs::getFunc(unsigned index) const {
    Function *ret;
    ret = calls.getFuncPtr(index);
    return ret;
}

LockUnlockPairs::lockUnlockPair *LockUnlockPairs::getPair(unsigned funcIndex, unsigned pairIndex) const {
    lockUnlockPair *pair;
    pair = NULL;
    if (funcIndex < funcPairs.size()) {
	if (pairIndex < funcPairs.at(funcIndex)->size()) {
	    pair = funcPairs.at(funcIndex)->at(pairIndex);
	}
#ifdef MUT_DEBUG
	else {
	    errs() << "DEBUG: LockUnlockPairs::getPair pairIndex out-of-bounds\n";
	}
#endif
    }
#ifdef MUT_DEBUG
    else {
	errs() << "DEBUG: LockUnlockPairs::getPair funcIndex out-of-bounds\n";
    }
#endif

    return pair;
}

void LockUnlockPairs::printDebugInfo() const {
    for (unsigned i = 0; i < getFuncsSize(); i++) {
	Function *curFunc;
	curFunc = getFunc(i);
	if (!curFunc) {
	    errs() << "Warning: lockPairs.getFunc() returned NULL, skipping\n";
	    continue;
	}

	errs() << curFunc->getName() << '\n';

	for (unsigned j = 0; j < funcPairs.at(i)->size(); j++) {
	    lockUnlockPair *curPair;
	    curPair = getPair(i, j);
	    if (!curPair) {
		errs() << "Warning, in LockUnlockPairs::printDebugInfo, getPair() "
			  "returned NULL, skipping\n";
		continue;
	    }
	    errs() << '\t' << *(curPair->lockCall) << "\n\t" << *(curPair->unlockCall)
		   << '\n';
	    MDNode *metaNode1;
	    MDNode *metaNode2;
	    metaNode1 = curPair->lockCall->getMetadata("dbg");
	    metaNode2 = curPair->unlockCall->getMetadata("dbg");

	    if (metaNode1) {
		DILocation Loc(metaNode1);
		StringRef File = Loc.getFilename();
		unsigned Line = Loc.getLineNumber();
		errs() << '\t' << File << ' ' << Line;
	    }
	    if (metaNode2) {
		errs() << '\t';
		DILocation Loc(metaNode2);
		StringRef File = Loc.getFilename();
		unsigned Line = Loc.getLineNumber();
		errs() << '\t' << File << ' ' << Line;
	    }

	    errs() << '\t' << calcDistanceBetween(curPair->lockCall, curPair->unlockCall) << '\n';

	    // If this is not the last iteration, output an extra newline to
	    // separate each of the pairs from each other
	    if (j == funcPairs.at(i)->size() - 1) {
		errs() << '\n';
	    }
	    else {
		errs() << "\n\n";
	    }
	}
    }
}

int LockUnlockPairs::calcDistanceBetween(Instruction *inst1, Instruction *inst2) {
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
