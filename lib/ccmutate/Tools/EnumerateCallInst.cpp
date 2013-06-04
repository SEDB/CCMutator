/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file EnumerateCallInst.cpp
 * \author Markus Kusano
 *
 * See EnumerateCallInst.h for more information
 */

#include "EnumerateCallInst.h"
#include "RemoveInst.h"
#include "ItaniumDemangle.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

//#define MUT_DEBUG

// Enable even more debugging output
//#define MUT_DEBUG_VERB

EnumerateCallInst::EnumerateCallInst() {
    isCpp = false;
}

void EnumerateCallInst::addFuncNameToSearch(std::string funcName) {
    funcNames.insert(funcName);
}

void EnumerateCallInst::visitCallInst(CallInst &I) {
    // Check if the function being called is in the search set
    Function *call;
    call = I.getCalledFunction();
    bool ret;

    ret = checkIfMatch(call);
    if (ret) {
#ifdef MUT_DEBUG
        errs() << "DEBUG: Matching CallInst found\n" << I << '\n';
#endif
        callInsts.push_back(&I);
    }
}

void EnumerateCallInst::visitInvokeInst(InvokeInst &I) {
    // Check if the function being called is in the search set
    Function *call;
    call = I.getCalledFunction();
    bool ret;

    ret = checkIfMatch(call);
    if (ret) {
#ifdef MUT_DEBUG
        errs() << "DEBUG: Matching InvokeInst found\n" << I << '\n';
#endif
        invokeInsts.push_back(&I);
    }
}

bool EnumerateCallInst::checkIfMatch(Function *F) {
    char *demangledFuncName;
    std::string foundName;

    if (F) {
	// Assumption: calls to searched for will never be indirect
#ifdef MUT_DEBUG_VERB
	errs() << "DEBUG: function calling: ";
	errs() << F->getName()<< '\n';
#endif
        if (isCpp) {
            demangledFuncName = demangleCpp(F->getName());  // potentially returns malloced char*
            if (demangledFuncName == NULL) {
                // If demangling failed, assume the function name is a non mangled
                foundName= F->getName();
            }
            else {
                foundName = removeParameters(demangledFuncName);
                free(demangledFuncName);
            }
        }
        else {
            foundName = F->getName();
        }

	if (funcNames.count(foundName)) {
	    // Found a match
            return true;
	}
    }
    else {
	errs () << "WARNING: Indirect function call found, ignoring\n";
    }

    return false;
}

void EnumerateCallInst::printInstDebugInfo() const {
    // Iterate over each  call instruction and print out
    // it's information if found. Otherwise print out a warning
    MDNode *metaNode;
    for (unsigned i = 0; i < callInsts.size(); i++) {
	metaNode = callInsts[i]->getMetadata("dbg");
	if (metaNode) {
	    DILocation Loc(metaNode);
	    StringRef File = Loc.getFilename();
	    unsigned Line = Loc.getLineNumber();
	    errs() << File << '\t' << Line << '\n';
	}
	else {
	    errs() << "Warning: No meta data found for instruction "
		   << i << '\n';
	}
    }
    for (unsigned i = 0; i < invokeInsts.size(); i++) {
        metaNode = invokeInsts[i]->getMetadata("dbg");
        if (metaNode) {
            DILocation Loc(metaNode);
            StringRef File = Loc.getFilename();
            unsigned Line = Loc.getLineNumber();
            errs() << File << '\t' << Line << '\n';
        }
        else {
            errs() << "Warning: No meta data found for instruction "
                   << i << '\n';
        }
    }
}

int EnumerateCallInst::removeFromParentRepZero(unsigned index, unsigned size, bool sign) {
    int errorCode;
    bool isCallInst;
    Instruction *curInst;

    curInst = getInstructionAt(index, &isCallInst, &errorCode);

    if (curInst == NULL) {
	return errorCode;
    }

    if (isCallInst) {
#ifdef MUT_DEBUG
        errs() << "DEBUG: Removing CallInst:\n\t" << *(callInsts[index]) << '\n';
#endif
        if (sign) {
            eraseFromParentOrReplace(callInsts[index], 0, sizeof(int), true);
        }
        else {
            eraseFromParentOrReplace(callInsts[index], 0, sizeof(unsigned), false);
        }
    }
    else if (!isCallInst) {
#ifdef MUT_DEBUG
        errs() << "DEBUG: Removing InvokeInst:\n\t" << *(invokeInsts[index - callInsts.size()])
               << '\n';
#endif
        if (sign) {
            eraseInvokeOrRep(invokeInsts[index - callInsts.size()], 0, sizeof(int), true);
        }
        else {
            eraseInvokeOrRep(invokeInsts[index - callInsts.size()], 0, sizeof(unsigned), false);
        }
    }
    else {
        errs() << "Warning: in EnumerateCallInst::removeFromParentRepZero(): "
                  "unknown return code from isValidIndex()\n";
    }


    deletedIndices.insert(index);
    return 0;
}

void EnumerateCallInst::eraseInst(unsigned index) {
    deletedIndices.insert(index);
    callInsts[index]->eraseFromParent();
#ifdef MUT_DEBUG
    errs() << "\tDEBUG: Index deleted\n";
#endif
}

int EnumerateCallInst::isValidIndex(unsigned index) {
    // Indexs are considered as if CallInsts and InvokeInsts were one
    // continuous array. 
    if (index >= callInsts.size() + invokeInsts.size()) {
#ifdef MUT_DEBUG
	errs() << "\tDEBUG: Index out of bounds\n";
#endif
	return -1; // index out of bounds
    }
    if (deletedIndices.count(index)) {
#ifdef MUT_DEBUG
	errs() << "\tDEBUG: Index already removed\n";
#endif
	return -2; // index already removed from parent.
    }

    if (index < callInsts.size()) {
        return 0;
    }
    else {
        return 1;
    }
}

/// Attempt to replace the instruction at the passed index with the
/// passed instruction.
///
/// \param index Index to attemp to replace
/// \param inst Instruction to replace the instruction at index with
/// \return Returns 0 on success, -1 if the index is out of bounds, -2
/// if the position has been modified already.
int EnumerateCallInst::replaceInstWithInst(unsigned index, Instruction *inst) {
#ifdef MUT_DEBUG
    errs() << "DEBUG: in EnumerateCallInst::replaceInstWithInst\n";
#endif
    Instruction *curInst;
    int errorCode;
    bool isCallInst;

    curInst = getInstructionAt(index, &isCallInst, &errorCode);

    if (curInst == NULL) {
        return errorCode;
    }
#ifdef MUT_DEBUG
    errs() << "DEBUG: replacing instruction:\n" << *curInst << "\nwith\n"
           << *inst << '\n';
#endif

    // Index is valid, so add the index to the deleted map and perform the
    // replacement
    deletedIndices.insert(index);

    // If we are dealing with an invoke instruction then it is a terminator for
    // the current basic block. Before doing the replacment, insert a branch to
    // the normal label of the invoke instruction as the new terminator
    if (!isCallInst) {
        BasicBlock *normDest;
        BranchInst *normBranch;
        normDest = ((InvokeInst *)curInst)->getNormalDest();
        normBranch = BranchInst::Create(normDest, curInst->getParent());
        (void) normBranch; // get rid of set but not used warning.
    }


    BasicBlock::iterator iter(curInst);
    ReplaceInstWithInst(curInst->getParent()->getInstList(), iter, inst);

    return 0;
}

int EnumerateCallInst::markMutated(unsigned index) {
    int ret = isValidIndex(index);

    if (ret < 0) {
	return ret;
    }
    deletedIndices.insert(index);

    return 0;
}

Instruction *EnumerateCallInst::getInstructionAt(unsigned index, bool *isCallInst, 
        int *errorCode) {
    int ret = isValidIndex(index);
    if (ret < 0) {
        if (errorCode != NULL) {
            *errorCode = ret;
        }
        return NULL;
    }
    if (ret == 0) {
        if (isCallInst != NULL) {
            *isCallInst = true;
        }
        return callInsts[index];
    }
    else if (ret == 1) {
        if (isCallInst != NULL) {
            *isCallInst = false;
        }
        return invokeInsts[index - callInsts.size()];
    }
    else {
        errs() << "Warning: in EnumerateCallInst::getInstructionAt(): unknown "
                  "return code from isValidIndex()\n";
    }
    return NULL;
}

int EnumerateCallInst::removeFromParent(unsigned index) {
#ifdef MUT_DEBUG
    errs() << "DEBUG: Attempting to remove index " << index << " from parent\n";
    errs() << "\tDEBUG: callInsts.size() == " << callInsts.size() << '\n';
    errs() << "DEBUG: callinst[index] has: " << callInsts[index]->getNumUses()
	   << " uses\n";
#endif
    int ret;
    ret = isValidIndex(index);
    if (ret) {
	return ret;
    }
    if (callInsts[index]->hasNUses(0)) {
	eraseInst(index);
    }
    else {
	return -3; // Instruction still has uses
    }


    return 0;
}

void EnumerateCallInst::searchCpp() {
    isCpp = true;
}

bool EnumerateCallInst::getIsCpp() {
    return isCpp;
}

