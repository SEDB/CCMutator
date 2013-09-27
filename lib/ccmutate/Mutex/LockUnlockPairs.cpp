/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 */
#include "LockUnlockPairs.h"
#include "../Tools/ItaniumDemangle.h"
#include "../Tools/FileInfo.h"

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"

#include <vector>
#include <string>

// Enable debugging output
#define MUT_DEBUG

// Enable verbose output
#define MUT_DEBUG_VERBOSE

void LockUnlockPairs::enumerate(Module &M, AliasAnalysis &AA) {
    // Iterate over every function
    Module::iterator fIter = M.begin();
    Module::iterator fEnd = M.end();

    for(;fIter != fEnd; ++fIter) {
#ifdef MUT_DEBUG_VERB
        errs() << "DEBUG: checking new function\n";
#endif
        // Vector to hold all lock & unlock call for the function
        std::vector<CallInst *> mutexCalls;
        std::vector<InvokeInst *> mutexInvokes;

        // Iterate over every instruction
	inst_iterator I;
	inst_iterator E;
	for (I = inst_begin(&*fIter), E = inst_end(&*fIter);
		I != E; ++I) {
            // Find target call/invoke instructions
	    if (CallInst* callInst = dyn_cast<CallInst>(&*I)) {
		Function *calledFunc = callInst->getCalledFunction();
		if (calledFunc) {
                    //StringRef funcName = calledFunc->getName();
                    bool matchedFuncCall;
                    matchedFuncCall = isMatch(calledFunc);
                    if (matchedFuncCall) {
                        mutexCalls.push_back(callInst);
                    }
		}
	    }
            else if (InvokeInst* invokeInst = dyn_cast<InvokeInst>(&*I)) {
		Function *calledFunc = invokeInst->getCalledFunction();
		if (calledFunc) {
                    //StringRef funcName = calledFunc->getName();
                    bool matchedFuncCall;
                    matchedFuncCall = isMatch(calledFunc);
                    if (matchedFuncCall) {
                        mutexInvokes.push_back(invokeInst);
                    }
		}
	    }
        } // end for (insts)
        // Find pairs in the current function
        //findCallPairs(mutexCalls, AA);
        //findInvokePairs(mutexInvokes, AA);
        findPairs(mutexCalls, mutexInvokes, AA);
    } // end for (funcs)
} // end func

bool LockUnlockPairs::isMatch(Function *func) {
    std::string noParams;
    bool ret;
    ret = false;
    noParams = getFunctionName(func);

#ifdef MUT_DEBUG_VERB
    errs() << "DEBUG: checking for match: " << noParams << '\n';
#endif

    if (noParams == "std::__1::mutex::unlock") {
        ret = true;
    }
    else if (noParams == "std::__1::mutex::lock") {
        ret = true;
    }
    else if (noParams == "pthread_mutex_lock") {
        ret = true;
    }
    else if (noParams == "pthread_mutex_unlock") {
        ret = true;
    }

#ifdef MUT_DEBUG_VERB
    errs() << "DEBUG: match found? " << ret << '\n';
#endif
    return ret;
}

#if 0 // These two functions don't provide full coverage of the case when a
      // lock is a CallInst and the unlock call is an Invoke and vice versa.
void LockUnlockPairs::findCallPairs(std::vector<CallInst *> &calls, AliasAnalysis &AA) {
#ifdef MUT_DEBUG
    errs() << "DEBUG: attempting to find call pairs in the following\n (size == " 
           << calls.size() << ")\n";
    for (unsigned i = 0; i < calls.size(); i++) {
        errs() << "\t" << *(calls[i]) << '\n';
    }
#endif

    // Check for a match in the unlock calls that follow a lock call. The
    // assumption is that a paired up unlock call cannot be before the
    // associated lock call.
    for (unsigned i = 0; i < calls.size(); i++) {
        CallInst *inst1;;
        inst1 = calls[i];
        if (isLockCall(inst1->getCalledFunction())) {
            for (unsigned j = i + 1; j < calls.size(); j++) {
                CallInst *inst2;
                inst2 = calls[j];
                if (isLockUnlockPair(inst1, inst2, AA)) {
#ifdef MUT_DEBUG
                    errs() << "DEBUG: found pair:\n\t" << *inst1 << "\n\t" << *inst2 << '\n';
#endif
                    CallLockUnlockPair *newPair;
                    newPair = new CallLockUnlockPair;
                    newPair->lockCall = inst1;
                    newPair->unlockCall = inst2;
                    CallPairs.push_back(newPair);
                }
            }
        }
    }
}

void LockUnlockPairs::findInvokePairs(std::vector<InvokeInst *> &calls, AliasAnalysis &AA) {
#ifdef MUT_DEBUG
    errs() << "DEBUG: attempting to find invoke pairs in the following\n (size == " 
           << calls.size() << ")\n";
    for (unsigned i = 0; i < calls.size(); i++) {
        errs() << "\t" << *(calls[i]) << '\n';
    }
#endif
    // Check for a match in the unlock calls that follow a lock call. The
    // assumption is that a paired up unlock call cannot be before the
    // associated lock call.
    for (unsigned i = 0; i < calls.size(); i++) {
        InvokeInst *inst1;;
        inst1 = calls[i];
        if (isLockCall(inst1->getCalledFunction())) {
            for (unsigned j = i + 1; j < calls.size(); j++) {
                InvokeInst *inst2;
                inst2 = calls[j];
                if (isLockUnlockPair(inst1, inst2, AA)) {
#ifdef MUT_DEBUG
                    errs() << "DEBUG: found pair:\n\t" << *inst1 << "\n\t" << *inst2 << '\n';
#endif
                    InvokeLockUnlockPair *newPair;
                    newPair = new InvokeLockUnlockPair;
                    newPair->lockCall = inst1;
                    newPair->unlockCall = inst2;
                    InvokePairs.push_back(newPair);
                }
            }
        }
    }
}
#endif

void LockUnlockPairs::findPairs(std::vector<CallInst *> &calls, std::vector<InvokeInst *> &invokes,
        AliasAnalysis &AA) {
    // For each lock instruction found in either calls or invokes compare it to
    // every other unlock call to see if they are a pair. There is probably a
    // more optimal way to do this by also comparing unlock calls to lock
    // calls.

    // Compare all the call instructions
    for (unsigned i = 0; i < calls.size(); i++) {
        CallInst *call1;
        call1 = calls[i];
        if (isLockCall(call1->getCalledFunction())) {
            // Compare to all other CallInsts
            for (unsigned j = 0; j < calls.size(); j++) {
                CallInst *call2;
                call2 = calls[j];
                if (call1 == call2) {
#ifdef MUT_DEBUG_VERB
                    errs() << "DEBUG: comparing call to it itself, skipping\n"
                           << "\t i == " << i << " j == " << j << '\n';
#endif
                    continue;
                }
                if (isLockUnlockPair(call1, call2, AA)) {
#ifdef MUT_DEBUG_VERB
                    errs() << "DEBUG: found pair:\n\t" << *call1 << "\n\t" << *call2 << '\n';
#endif
                    CallCallLockPair *newPair;
                    newPair = new CallCallLockPair;
                    newPair->lockCall = call1;
                    newPair->unlockCall = call2;
                    CallCallPairs.push_back(newPair);
                }
            } // end for
            for (unsigned j = 0; j < invokes.size(); j++) {
                // Compare to invoke instructions
                InvokeInst *invoke2;
                invoke2 = invokes[j];
                if (isLockUnlockPair(call1, invoke2, AA)) {
#ifdef MUT_DEBUG_VERB
                    errs() << "DEBUG: found pair:\n\t" << *call1 << "\n\t" << *invoke2 << '\n';
#endif
                    CallInvokeLockPair *newPair;
                    newPair = new CallInvokeLockPair;
                    newPair->lockCall = call1;
                    newPair->unlockInvoke = invoke2;
                    CallInvokePairs.push_back(newPair);
                }
            }
        }
    } // end for

    // Compare all the invokes
    for (unsigned i = 0; i < invokes.size(); i++) {
        InvokeInst *invoke1;
        invoke1 = invokes[i];
        if (isLockCall(invoke1->getCalledFunction())) {
            // Compare to all other CallInsts
            for (unsigned j = 0; j < calls.size(); j++) {
                CallInst *call2;
                call2 = calls[j];
                if (isLockUnlockPair(invoke1, call2, AA)) {
#ifdef MUT_DEBUG_VERB
                    errs() << "DEBUG: found pair:\n\t" << *invoke1 << "\n\t" << *call2 << '\n';
#endif
                    InvokeCallLockPair *newPair;
                    newPair = new InvokeCallLockPair;
                    newPair->lockInvoke = invoke1;
                    newPair->unlockCall = call2;
                    InvokeCallPairs.push_back(newPair);
                }
            } // end for
            for (unsigned j = 0; j < invokes.size(); j++) {
                // Compare to invoke instructions
                InvokeInst *invoke2;
                invoke2 = invokes[j];
                if (invoke1 == invoke2) {
#ifdef MUT_DEBUG_VERB
                    errs() << "DEBUG: comparing invoke to it itself, skipping\n"
                           << "\t i == " << i << " j == " << j << '\n';
#endif
                    continue;
                }
                if (isLockUnlockPair(invoke1, invoke2, AA)) {
#ifdef MUT_DEBUG_VERB
                    errs() << "DEBUG: found pair:\n\t" << *invoke1 << "\n\t" << *invoke2 << '\n';
#endif
                    InvokeInvokeLockPair *newPair;
                    newPair = new InvokeInvokeLockPair;
                    newPair->lockInvoke = invoke1;
                    newPair->unlockInvoke = invoke2;
                    InvokeInvokePairs.push_back(newPair);
                }
            } // end for
        }
    } // end for
}
bool LockUnlockPairs::isLockCall(Function *func) const {
    StringRef funcName;
    std::string noParams;
    bool ret = false;
    if (func == NULL) {
        return false; // indirect function calls are not resolved
    }

    noParams = getFunctionName(func);

    if (noParams == "pthread_mutex_lock") {
        ret = true;
    }
    else if (noParams == "std::__1::mutex::lock") {
        ret = true;
    }

    return ret;
}

bool LockUnlockPairs::isLockUnlockPair(Function *lockFunc, Function *otherFunc, 
        AliasAnalysis &AA, Value *mut1, Value *mut2) {
    if (lockFunc == NULL) {
        return false;
    }
    if (otherFunc == NULL) {
        return false;
    }
    if (isLockCall(otherFunc)) {
        return false;
    }
    // OPTIMIZATION: This can probably be removed.
    if (!isLockCall(lockFunc)) {
        errs() << "Warning: non lock call passed as lockFunc to isLockUnlockPair()\n";
        return false;
    }

    std::string lockFuncName;
    std::string otherFuncName;
    otherFuncName = getFunctionName(otherFunc);
    lockFuncName = getFunctionName(lockFunc);


    AliasAnalysis::AliasResult res;
    res = AA.alias(mut1, mut2);

    if (lockFuncName == "pthread_mutex_lock" && otherFuncName == "pthread_mutex_unlock") {
        // The calls are both to posix locks. The first parameters is the mutex
        if (res == AliasAnalysis::MustAlias) {
            return true;
        }
    }
    else if (lockFuncName == "std::__1::mutex::lock" && otherFuncName == "std::__1::mutex::unlock") {
        if (res == AliasAnalysis::MustAlias) {
            return true;
        }
    }

    return false;
}

bool LockUnlockPairs::isLockUnlockPair(CallInst *lockCall, CallInst *otherCall, AliasAnalysis &AA) {
    if (lockCall->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex call with < 1 operand, skipping\n";
        return false;
    }
    if (otherCall->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex call with < 1 operand, skipping\n";
        return false;
    }
    Value *mut1;
    Value *mut2;
    Function *lockFunc;
    Function *otherFunc;

    mut1 = lockCall->getArgOperand(0);
    mut2 = otherCall->getArgOperand(0);
    lockFunc = lockCall->getCalledFunction();
    otherFunc = otherCall->getCalledFunction();

    return isLockUnlockPair(lockFunc, otherFunc, AA, mut1, mut2);
}

bool LockUnlockPairs::isLockUnlockPair(InvokeInst *lockCall, InvokeInst *otherCall, AliasAnalysis &AA) {
    if (lockCall->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex invoke with < 1 operand, skipping\n";
        return false;
    }
    if (otherCall->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex invoke with < 1 operand, skipping\n";
        return false;
    }
    Value *mut1;
    Value *mut2;
    Function *lockFunc;
    Function *otherFunc;

    mut1 = lockCall->getArgOperand(0);
    mut2 = otherCall->getArgOperand(0);
    lockFunc = lockCall->getCalledFunction();
    otherFunc = otherCall->getCalledFunction();

    return isLockUnlockPair(lockFunc, otherFunc, AA, mut1, mut2);
}

bool LockUnlockPairs::isLockUnlockPair(CallInst *lockCall, InvokeInst *otherInvoke, AliasAnalysis &AA) {
    if (lockCall->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex call with < 1 operand, skipping\n";
        return false;
    }
    if (otherInvoke->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex invoke with < 1 operand, skipping\n";
        return false;
    }
    Value *mut1;
    Value *mut2;
    Function *lockFunc;
    Function *otherFunc;

    mut1 = lockCall->getArgOperand(0);
    mut2 = otherInvoke->getArgOperand(0);
    lockFunc = lockCall->getCalledFunction();
    otherFunc = otherInvoke->getCalledFunction();

    return isLockUnlockPair(lockFunc, otherFunc, AA, mut1, mut2);
}

bool LockUnlockPairs::isLockUnlockPair(InvokeInst *lockInvoke, CallInst *otherCall, AliasAnalysis &AA) {
    if (lockInvoke->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex call with < 1 operand, skipping\n";
        return false;
    }
    if (otherCall->getNumArgOperands() < 1) {
        errs() << "Warning: found a pthread or std::mutex invoke with < 1 operand, skipping\n";
        return false;
    }
    Value *mut1;
    Value *mut2;
    Function *lockFunc;
    Function *otherFunc;

    mut1 = lockInvoke->getArgOperand(0);
    mut2 = otherCall->getArgOperand(0);
    lockFunc = lockInvoke->getCalledFunction();
    otherFunc = otherCall->getCalledFunction();

    return isLockUnlockPair(lockFunc, otherFunc, AA, mut1, mut2);
}

unsigned LockUnlockPairs::getNumPairs() const {
    int ret;
    ret = 0;
    ret += CallCallPairs.size();
    ret += CallInvokePairs.size();
    ret += InvokeCallPairs.size();
    ret += InvokeInvokePairs.size();

    return ret;
}

unsigned LockUnlockPairs::getNumCallCallPairs() const {
    return CallCallPairs.size();
}
unsigned LockUnlockPairs::getNumCallInvokePairs() const {
    return CallInvokePairs.size();
}
unsigned LockUnlockPairs::getNumInvokeCallPairs() const {
    return InvokeCallPairs.size();
}

unsigned LockUnlockPairs::getNumInvokeInvokePairs() const {
    return InvokeInvokePairs.size();
}

void LockUnlockPairs::printDebugInfo() const {
    for (unsigned i = 0; i < CallCallPairs.size(); i++) {
        CallCallLockPair *curPair;
        CallInst *lockCall;
        CallInst *unlockCall;
        Function *parent;

        curPair = CallCallPairs[i];
        lockCall = curPair->lockCall;
        unlockCall = curPair->unlockCall;

        parent = lockCall->getParent()->getParent();
        errs() << parent->getName() << '\t' << 0 << '\t' << i << '\n';
        errs() << '\t' << *lockCall << '\n';
        errs() << '\t' << *unlockCall << '\n';


        printDebugInfo(lockCall, unlockCall);
    }
    for (unsigned i = 0; i < CallInvokePairs.size(); i++) {
        CallInvokeLockPair *curPair;
        CallInst *lockCall;
        InvokeInst *unlockInvoke;
        Function *parent;

        curPair = CallInvokePairs[i];
        lockCall = curPair->lockCall;
        unlockInvoke = curPair->unlockInvoke;

        parent = lockCall->getParent()->getParent();
        errs() << parent->getName() << '\t' << 1 << '\t' << i << '\n';
        errs() << '\t' << *lockCall << '\n';
        errs() << '\t' << *unlockInvoke << '\n';

        printDebugInfo(lockCall, unlockInvoke);
    }
    for (unsigned i = 0; i < InvokeCallPairs.size(); i++) {
        InvokeCallLockPair *curPair;
        InvokeInst *lockInvoke;
        CallInst *unlockCall;
        Function *parent;

        curPair = InvokeCallPairs[i];
        lockInvoke = curPair->lockInvoke;
        unlockCall = curPair->unlockCall;

        parent = lockInvoke->getParent()->getParent();
        errs() << parent->getName() << '\t' << 2 << '\t' << i << '\n';
        errs() << '\t' << *lockInvoke << '\n';
        errs() << '\t' << *unlockCall << '\n';
        
        printDebugInfo(lockInvoke, unlockCall);
    }
    for (unsigned i = 0; i < InvokeInvokePairs.size(); i++) {
        InvokeInvokeLockPair *curPair;
        InvokeInst *lockInvoke;
        InvokeInst *unlockInvoke;
        Function *parent;

        curPair = InvokeInvokePairs[i];
        lockInvoke = curPair->lockInvoke;
        unlockInvoke = curPair->unlockInvoke;

        parent = lockInvoke->getParent()->getParent();
        errs() << parent->getName() << 3 << '\t' << i << '\n';
        errs() << '\t' << *lockInvoke << '\n';
        errs() << '\t' << *unlockInvoke << '\n';

        printDebugInfo(lockInvoke, unlockInvoke);
    }
}

int LockUnlockPairs::calcDistanceBetween(Instruction *inst1, Instruction *inst2) const {
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

void LockUnlockPairs::printDebugInfo(Instruction *lockCall, Instruction *unlockCall) const {
    // Lock call filename and line number
    StringRef fileName1 = getDebugFilename(lockCall);
    unsigned lineNum1 = getDebugLineNum(lockCall);
    if (fileName1 == "") {
        errs() << "\tno debug filename for lock call";
    }
    else {
        errs() << '\t' << fileName1;
    }
    if (lineNum1 == UINT_MAX) {
        errs() << " no debug line number for lock call";
    }
    else {
        errs() << ' ' << lineNum1;
    }

    // Unlock call filename and line number
    StringRef fileName2 = getDebugFilename(unlockCall);
    unsigned lineNum2 = getDebugLineNum(unlockCall);
    if (fileName2 == "") {
        errs() << "\tno debug filename for lock call";
    }
    else {
        errs() << '\t' << fileName2;
    }
    if (lineNum1 == UINT_MAX) {
        errs() << " no debug line number for lock call";
    }
    else {
        errs() << ' ' << lineNum2;
    }

    // Distance between the two calls
    int distBetween;
    distBetween = calcDistanceBetween(lockCall, unlockCall);
    if (distBetween < 0 ) {
        errs() << "\tunable to calc distance between insts\n";
    }
    else {
        errs() << '\t' << distBetween << '\n';
    }
}

LockUnlockPairs::CallCallLockPair *LockUnlockPairs::getCallCallPair(unsigned index) {
    if (index < CallCallPairs.size()) {
        return CallCallPairs[index];
    }
    else {
        return NULL;
    }
}

LockUnlockPairs::CallInvokeLockPair *LockUnlockPairs::getCallInvokePair(unsigned index) {
    if (index < CallInvokePairs.size()) {
        return CallInvokePairs[index];
    }
    else {
        return NULL;
    }
}

LockUnlockPairs::InvokeCallLockPair *LockUnlockPairs::getInvokeCallPair(unsigned index) {
    if (index < InvokeCallPairs.size()) {
        return InvokeCallPairs[index];
    }
    else {
        return NULL;
    }
}

LockUnlockPairs::InvokeInvokeLockPair *LockUnlockPairs::getInvokeInvokePair(unsigned index) {
    if (index < InvokeInvokePairs.size()) {
        return InvokeInvokePairs[index];
    }
    else {
        return NULL;
    }
}
