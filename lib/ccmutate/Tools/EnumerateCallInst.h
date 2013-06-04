/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file EnumerateCallInst.h
 * \author Markus Kusano
 *
 * Generic visitor to CallInst that fills a data structure with pointers to
 * CallInsts that match the names in the set funcNames.
 *
 * Currently the code does not support indirect function calls. Alias analysis
 * is not implemented to resolve the function pointer, this is a work in
 * progress.
 *
 * The class works for CallInsts and InvokeInsts. In order to preserve the type
 * information there are two internal data structures to hold either CallInsts
 * or InvokeInsts. Most member functions work on indices that consider the two
 * data structures as one array starting with callInsts and going to
 * invokeInsts.
 */
#pragma once
#include "llvm/Support/InstVisitor.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include <vector>
#include <string>

using namespace llvm;

class EnumerateCallInst : public InstVisitor<EnumerateCallInst> {
    public:
        EnumerateCallInst();
	/// Data structure to hold pointers to call instructions
	std::vector<CallInst *> callInsts;

        /// Data structure to hold pointers to invoke instruction
        std::vector<InvokeInst *> invokeInsts;

	/// Data structure to hold input list of functions names to enumerate
	/// The initial size can be adjusted as an optimization
	SmallSet<std::string, 64> funcNames;

	/// Adds a function name to funcNames to be searched for when visit()
	/// is called.
	/// \param funcName Function name for visitor to search for.
	void addFuncNameToSearch(std::string funcName);

	/// Overridden visitor function for call and invoke instructions
	void visitCallInst(CallInst &I);
        void visitInvokeInst(InvokeInst &I);

        /// Returns a pointer to the instruction at the given index. This
        /// considers callInsts and invokeInsts as one array that starts at
        /// index 0 in callInsts through invokeInsts. Returns NULL if the index
        /// is out of bounds. Optional parameter isCallInst will be set to true
        /// if the instruction returned is a CallInst otherwise it is an
        /// InvokeInst. If it is NULL it is unmodified. If errorCode is non
        /// NULL then it will be -1 if the index is out of bounds or -2 if the
        /// index has been deleted already.
        Instruction *getInstructionAt(unsigned index, bool *isCallInst = NULL, 
                                        int *errorCode = NULL);

	/// Print out each found instructions debug information to stderr
	void printInstDebugInfo() const;

	/// Remove occurrence from its parent (remove them from the code) that
	/// are specified by the passed index. The index is an index of
	/// callInsts. If the index is out of bounds it is ignored. 
	/// The CallInst at index is removed from its parent but it is still
	/// retained in the data structure. 
	/// 
	/// \param index Index of instruction to remove from parent
	/// \return Returns 0 on success. Returns -1 if the index is out of
	/// bounds. Returns -2 if the position has been removed from its parent
	/// already . Returns -3 if the instruction to remove has more than 1
	/// use
	int  removeFromParent(unsigned index);

	/// Attempt to remove the occurrence at index. If the index has more
	/// uses, replace the instruction with a constant 32 bit zero.
	///
	/// \param index Index of instruction to remove from parent
	/// \param size in bytes of the replacement
	/// \param true if the replacment should be signed, false if unsigned
	/// \return Returns 0 on success. Returns -1 if the index is out of
	/// bounds. Returns -2 if the position has been removed from its parent
	/// already. 
	int removeFromParentRepZero(unsigned index, unsigned size, bool sign);

	/// Attempt to replace the instruction at the passed index with the
        /// passed instruction. If the instruction at index is an InvokeInst
        /// then a branch instruction to the normal destination of the invoke
        /// instruction is inserted as the terminator to the basic block before
        /// the replacment is done. If inst is another terminator, this could
        /// lead to a basic block that has two terminators, inst should not be
        /// a terminating instruction if index is an InvokeInst.
	///
	/// \param index Index to attemp to replace
	/// \param inst Instruction to replace the instruction at index with
	/// \return Returns 0 on success, -1 if the index is out of bounds, -2
	/// if the position has been modified already.
	int replaceInstWithInst(unsigned index, Instruction *inst);

        /// Checks if the passed index is a valid index to modify. This means
        /// that it is inbounds and has not been deleted yet.  
        /// \param index to check 
        /// \return -1 if the index is out of bounds and -2 if the index
        /// has already been deleted. Returns 0 if the index is in CallInsts or
        /// 1 if the index is in InvokeInsts
	int isValidIndex(unsigned index);

	/// Adds the index to deletedIndices.
	/// \param index to add to deletedIndices
	/// \return Returns 0 if the index was added, -1 if the index is out of
	/// bounds and -2 if the index has already been mutated.
	int markMutated(unsigned index);

        /// Call to enable searching for Cpp functions. Encountered CallInsts
        /// will have their called function name demangled and then compared
        /// against the functions being searched for.
        void searchCpp();

        /// Returns true if we are searching for Cpp functions.
        bool getIsCpp();

    private:
	/// Set of indecies that have been removed from their parent. This
	/// becomes invalid if callInsts has one or more of its values removed.
	/// This is used to ensure that the same instruction is not removed
	/// from its parent twice.
	SmallSet<int, 64> deletedIndices;

	/// Erase the instruction at the passed index from it's parent. No
	/// bounds checking is done to ensure that the index is inbounds of the
	/// callInst vector. The index is added to the deletedIndicies map
	/// \param index Index to remove from it's parent.
	void eraseInst(unsigned index);

        /// Check if the passed function is on being searched for. Return true
        /// if it is, otherwise false.
        bool checkIfMatch(Function *F);

        bool isCpp;
};
