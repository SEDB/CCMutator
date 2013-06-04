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
#pragma once
#include "llvm/Instructions.h"

using namespace llvm;

/// Remove the passed instruction from its parent if it has no uses. Otherwise,
/// replace it with value of the passed size (in bytes) and either signed or
/// unsigned.
int eraseFromParentOrReplace(Instruction *inst, int value, unsigned size, bool sign);

/// Replace the passed invoke instruction with a branch to its normal label if
/// it has no uses. Otherwise replace it with value of size size and either
/// signed or unsigned.
int eraseInvokeOrRep(InvokeInst *inst, int value, unsigned size, bool sign);
