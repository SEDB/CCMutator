/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 */

#include "AliasResultToString.h"
#include "llvm/Support/raw_ostream.h"

const char *AAResToString(AliasAnalysis::AliasResult res) {
    if (res == AliasAnalysis::NoAlias) {
	return "No Alias";
    }
    else if (res == AliasAnalysis::MayAlias) {
	return "May Alias";
    }
    else if (res == AliasAnalysis::PartialAlias) {
	return "Partial Alias";
    }
    else if (res == AliasAnalysis::MustAlias) {
	return "Must Alias";
    }
    else {
	errs() << "Warning: in AAResToString, unknown alias result encountered\n";
	return "Unknown alias result";
    }
}

