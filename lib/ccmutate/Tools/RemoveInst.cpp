/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file RemoveInst.h
 * \author Markus Kusano
 *
 * Collection of functions to safely remove instructions from their parent.
 * This is mainly used to ensure that if an instruction has uses when it is to
 * be removed it is replaced with something appropriate
 */
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/LLVMContext.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/BasicBlock.h"

#include "RemoveInst.h"

// Enable debugging output
#define MUT_DEBUG

/// Remove the passed instruction from its parent if it has no uses. Otherwise,
/// replace it with value of the passed size (in bytes) and either signed or
/// unsigned.
int eraseFromParentOrReplace(Instruction *inst, int value, unsigned size, bool sign) {
    if (inst->hasNUses(0)) {
	inst->eraseFromParent();
    }
    else {
	BasicBlock::iterator iter(inst);
	LLVMContext &con = getGlobalContext();

	// Index has uses, replace it with a zero
	ReplaceInstWithValue(inst->getParent()->getInstList(), iter,
		ConstantInt::get(Type::getIntNTy(con, size * 8), value, sign));
    }

    return 0;
}

int eraseInvokeOrRep(InvokeInst *inst, int value, unsigned size, bool sign) {
#ifdef MUT_DEBUG
    errs() << "DEBUG: attempting to erase InvokeInst with " << inst->getNumUses() << " uses\n";
#endif
    // Insert a branch instruction at the end of the basic block containing the
    // invoke instruction. This implicitly inserts it after the invoke
    // instruction.
    BasicBlock *normDest;
    BranchInst *normBranch;
    normDest = inst->getNormalDest();
    normBranch = BranchInst::Create(normDest, inst->getParent());
    (void) normBranch; // get rid of set but not used warning.

    return eraseFromParentOrReplace(inst, value, size, sign);
}
