/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file VolatileVisitor.h
 * Author: Markus Kusano
 * Date: 2013-01-30
 *
 * Visitor to check if certain types of instructions are volatile.
 */

#include "llvm/Support/InstVisitor.h"
#include <vector>

using llvm::InstVisitor;
using llvm::LoadInst;
using llvm::StoreInst;
using llvm::AtomicCmpXchgInst;
using llvm::AtomicRMWInst;
using llvm::MemCpyInst;
using llvm::MemMoveInst;
using llvm::MemSetInst;

using llvm::Instruction;

using std::vector;

class VolatileVisitor: public InstVisitor<VolatileVisitor> {
    public:
	// Functions to visit instructions that potentially can be volatile
	void visitLoadInst(LoadInst &I);
	void visitStoreInst(StoreInst &I);
	void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I);
	void visitAtomicRMWInst(AtomicRMWInst &I);
	void visitMemCpyInst(MemCpyInst &I);
	void visitMemMoveInst(MemMoveInst &I);
	void visitMemSetIsnt(MemSetInst &I);

	// Functions to obtain information about the data structure
	/// \return the size of the data structure volaInsts
	unsigned getVolaInstsSize() const;

	/// \param index of value to obtain
	/// \return obtain the value at index 
	Instruction *getVolaInst(unsigned int index) const;
    private:
	/// Data structure to hold pointers to the instructions found to be
	/// volatile. Declared with an initial size of 64. Once more than 64
	/// items are added, a resize needs to be performed.
	//
	/// OPTIMIZATION: The size can be changed to use more memory but
	/// decrease the potential for a resize.
	vector<Instruction *> volaInsts;
};
