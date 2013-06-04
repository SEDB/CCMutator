/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file IantiumDemangle.h
 * \author Markus Kusano
 *
 * Demangles a C++ function call that uses the Iantium ABI.
 *
 * This is essentially a wrapper around libstdc++'s abi::__cxa_demangle and as
 * a result requires libstdc++.
 */
#pragma once
#include <cxxabi.h>
#include "llvm/ADT/StringRef.h"
#include "llvm/Function.h"

using namespace llvm;

/// Returns a demangled version of the passed StringRef. The returned char* is
/// malloced and it is up to the user to free it. If an error occurs, it is
/// output to stderr and NULL is returned (no malloc occurs).
char *demangleCpp(StringRef name);

/// Removes all characters including and after the first '(' in the
/// passed string. Returns a default constructed string on error.
std::string removeParameters(char *funcCall);

// Returns the function name with no paremters. Attempts to demangle the
// function call if possible. If the demangle fails, it is assumed to be an
// unmangled name. Returns a default constructed std::string on failure.
std::string getFunctionName(Function *func);

