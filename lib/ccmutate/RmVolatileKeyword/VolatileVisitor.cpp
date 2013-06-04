/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file VolatileVisitor.cpp
 * Author: Markus Kusano
 * Date: 2013-01-30
 *
 * Visitor functions for instructions that potentially could be volatile
 */
#include "VolatileVisitor.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"

using llvm::ConstantInt;
using llvm::Value;
using llvm::dyn_cast;
using llvm::errs;

void VolatileVisitor::visitLoadInst(LoadInst &I) {
    if (I.isVolatile()) {
	volaInsts.push_back(&I);
    }
}

void VolatileVisitor::visitStoreInst(StoreInst &I) {
    if (I.isVolatile()) {
	volaInsts.push_back(&I);
    }
}

void VolatileVisitor::visitAtomicCmpXchgInst(AtomicCmpXchgInst &I) {
    if (I.isVolatile()) {
	volaInsts.push_back(&I);
    }
}

void VolatileVisitor::visitAtomicRMWInst(AtomicRMWInst &I) {
    if (I.isVolatile()) {
	volaInsts.push_back(&I);
    }
}

void VolatileVisitor::visitMemCpyInst(MemCpyInst &I) {
    // The 5th parameter of a llvm.memcpy instruction is a bool which is true
    // if the instruction is volatile. These can be accessed using inherited
    // member functions from llvm::CallInst
    Value *param5;

    if (I.getNumArgOperands() >= 5) {
	param5 = I.getArgOperand(4);
	if (ConstantInt *ci = dyn_cast<ConstantInt>(param5)) {
	    if (ci->isOne()) {
		DEBUG(errs() << "DEBUG: volatile llvm.memcpy instruction found\n");
		volaInsts.push_back(&I);
	    }
	}
    }
    else {
	errs() << "Warning: Encountered llvm.memcpy inst with less than five parameters\n";
    }

    DEBUG(errs() << "DEBUG: llvm.memcpy instruction visited");
}

void VolatileVisitor::visitMemMoveInst(MemMoveInst &I) {

}

void VolatileVisitor::visitMemSetIsnt(MemSetInst &I) {

}


unsigned VolatileVisitor::getVolaInstsSize() const {
    return volaInsts.size();
}

Instruction *VolatileVisitor::getVolaInst(unsigned int index) const {
    if (index >= volaInsts.size())
	return NULL;

    return volaInsts[index];
}
