/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 */
#pragma once
#include "llvm/Analysis/AliasAnalysis.h"

using namespace llvm;

/// Converts the passed alias result to a string form
const char *AAResToString(AliasAnalysis::AliasResult res);

