/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file FuncLocalLockCalls.h
 * \author Markus Kusano
 * \date 2013-03-07
 *
 * This class takes as input a pointer to a module and produces a set of
 * llvm:Function *'s and the corresponding calls to pthread_mutex_lock and
 * pthread_mutex_unlock in order that exist in that function.
 */
#pragma once
#include <vector>
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"

using namespace llvm;

class FuncLocalLockCalls {
    public:
	/// Populates the internal data structures by searching the functions
	/// in passed Module
	void search(Module &M);

	/// Returns the Function * at the passed index. If the index is out of
	/// bounds, returns NULL
	/// \param index Index of function to obtain
	/// \ret returns Function * at index or NULL if out of bounds
	Function *getFuncPtr(unsigned index) const;

	/// Returns the CallInst at the given funcIndex, callIndex. If either
	/// index is out of bounds, the function returns NULL
	/// \param funcIndex function index to obtain the CallInst
	/// \param callInst index of CallInst in function to obtain
	/// \ret returns CallInst at the passed position, or NULL if either if
	/// out of bounds
	CallInst *getCallInstPtr(unsigned funcIndex, unsigned callIndex);

	/// Debug function. Dumps the contents of the internal data structures
	/// to stderr
	void dump();

	/// \ret The size of the internal funcs data structure
	unsigned getFuncsSize() const;

	/// \ret The size of the internal calls data structure 
	unsigned getCallsSize() const;

	/// \ret The size of the CallInst vector inside calls at the passsed
	/// index. Returns 0 if the index is out of bounds
	unsigned getCallsSizeAt(unsigned index) const;

    private:
	// A vector to hold pointers to functions that have been search. This
	// vector lines up with the CallInst vector
	std::vector<Function *> funcs;

	// A vector of pointers to vectors of CallInst *'s. The top vector
	// corresponds to indecies in funcs. The bottom vector contains the
	// calls to pthread_mutex_lock and pthread_mutex_unlock that are found
	// in the function in the order that they occur.
	std::vector<std::vector<CallInst *>* > calls;
};
