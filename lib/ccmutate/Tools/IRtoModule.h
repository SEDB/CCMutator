/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 *
 * \file IRtoModule.h
 * \author Markus Kusano
 *
 * Simply takes an std::string to a filename and returns a Module* to the
 * parsed code.
 *
 * It appears that the extra passed SMDiagnostic parameter is simply extra
 * information if an error occurs. This information is printed out via stderr.
 */
#pragma once
#include <llvm/Support/IRReader.h>
#include <llvm/Support/raw_ostream.h>

using namespace::llvm;

/// Attempts to parse filename using context. progName is the currently
/// executing program name. This is used for outputting error messages.
/// 
/// \param filename LLVM IR file to parse
/// \param context LLVM Context to parse with
/// \param progName currently executing program name. Used for error reporting
/// \return Returns a parsed LLVM module or NULL on failure.
Module *IRtoModule(std::string filename, LLVMContext &context, char* progName);
