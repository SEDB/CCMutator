/**
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE for details.
 */

#include "ItaniumDemangle.h"
#include "llvm/Support/raw_ostream.h"
#include <cstring>


//#define MUT_DEBUG

// Enable ever more debugging output
//#define MUT_DEBUG_VERB

char *demangleCpp(StringRef name) {
    if (name.size() == 0) {
        errs() << "Warning: name of size 0 passed to demangleCpp()\n";
        return NULL;
    }
#ifdef MUT_DEBUG_VERB
    errs() << "DEBUG: in demangleCpp:\n\tname.size() == " << name.size() << '\n';
#endif

    size_t ret;
    ret = name.find('\0');
    if (ret != StringRef::npos) {
        errs() << "Warning: name with embedded NULL character passed to demangleCpp\n";
        return NULL;
    }
#ifdef MUT_DEBUG
    errs() << "DEBUG: attempting to demangle " << name << '\n';
#endif

    const char *nameData;
    char *nameCStr;
    char *demangled; // this is malloced by __cxa_demangle
    int status;

    nameData = name.data();

#ifdef MUT_DEBUG_VERB
    errs() << "\tin demangleCpp: nameCStr[name.size() - 1] == "
           << nameData[name.size() -1] << '\n';
#endif
    // Check if the data is NULL terminated, if not, add one
    if (nameData[name.size() - 1] != '\0') {
#ifdef MUT_DEBUG_VERB
        errs() << "\tString is not null terminated\n";
#endif
        nameCStr = (char *)malloc(sizeof(char) * name.size() + 1);
        if (nameCStr == NULL) {
            errs() << "Error: malloc() failed in demangleCpp()\n";
            exit(EXIT_FAILURE);
        }
        memcpy((void *)nameCStr, (void *)name.data(), name.size());
        nameCStr[name.size()] = '\0';
    }
    else {
#ifdef MUT_DEBUG_VERB
        errs() << "\tString is null terminated\n";
#endif
        nameCStr = (char *) malloc(sizeof(char) * name.size());
        if (nameCStr == NULL) {
            errs() << "Error: malloc() failed in demangleCpp()\n";
            exit(EXIT_FAILURE);
        }
        memcpy((void *)nameCStr, (void *)name.data(), name.size());
    }
    demangled = abi::__cxa_demangle(nameCStr, NULL, NULL, &status);
#ifdef MUT_DEBUG
    if (demangled != NULL) {
        errs() << "DEBUG: demangled name == " << demangled << '\n';
    }
    else {
        errs() << "DEBUG: demangled name == NULL\n";
    }
    errs() << "\tstatus == " << status << '\n';
#endif
    free(nameCStr);

    if (!status) {
        return demangled;
    }
    else if (status == -1) {
        errs() << "Error: malloc failed in demangleCpp()\n";
        exit(EXIT_FAILURE);
    }
    else if (status == -2) {
        //errs() << "Warning: non valid mangled name passed to demangleCpp()\n";
        // Be silent when the name is invalid to be demangled. This happens alot.
        return NULL;
    }
    else if (status == -3) {
        errs() << "Warning: invalid argument passed to __cxa_demangle in demangleCpp\n";
        return NULL;
    }
    errs() << "Warning: unknown return code of __cxa_demangle encounted in demangleCpp\n";
    return NULL;
}

std::string removeParameters(char *funcName) {
    std::string ret;
    size_t parenPos;

    ret = funcName;
    parenPos = ret.find('(');

#ifdef MUT_DEBUG_VERB
    errs() << "DEBUG: parenthesis found at: " << parenPos << '\n';
#endif
    if (parenPos == std::string::npos) {
        // no paren found
        return ret;
    }
    ret = ret.substr(0, parenPos);

#ifdef MUT_DEBUG
    errs() << "DEBUG: non parenthesis function call: " << ret << '\n';
#endif

    // Remove template parameters
    parenPos = ret.find('<');
    if (parenPos != std::string::npos) {
        ret = ret.substr(0, parenPos);
    }

#ifdef MUT_DEBUG
    errs() << "DEBUG: non template function call: " << ret << '\n';
#endif

    // Remove return value
    parenPos = ret.find(' ');
    if (parenPos != std::string::npos) {
        ret = ret.substr(parenPos + 1, ret.size());
    }

#ifdef MUT_DEBUG
    errs() << "DEBUG: non return function call: " << ret << '\n';
#endif

    return ret;
}

std::string getFunctionName(Function *func) {
    StringRef funcName;
    std::string ret;
    char *demangled;
    if (func == NULL) {
        return ret; // indirect function calls are not resolved
    }

    funcName = func->getName();
    demangled = demangleCpp(funcName);  // malloc'd

    if (demangled == NULL) {
        ret = funcName; // non demangled names have no parameters
    }
    else {
        ret = removeParameters(demangled);
        free(demangled);
    }

    return ret;
}

