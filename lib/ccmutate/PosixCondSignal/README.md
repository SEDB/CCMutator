## Posix Cond Signal Readme

### Description
Mutation operator working on `pthread_cond_signal()` and
`pthread_cond_broadcast()` 

The following operations are supported:

1. Remove call to `pthread_cond_signal()`
1. Remove call to `pthread_cond_broadcast()`
1. Replace call to `pthread_cond_signal()` with `pthread_cond_broadcast()`
1. Replace call to `pthread_cond_broadcast()` with `pthread_cond_signal()`

### Usage
By default (no command parameters) and if `opt` is passed the `-analyze` flag
then the number of occurances of `pthread_cond_signal()` and
`pthread_cond_broadcast` will be sent to `stderr`.

Example:
`````
opt -analyze -load $LLVM_LIB_DIR/mutate_PosixCondSignal.so -PosixCondSignal <test.bc >/dev/null
`````

#### Verbose Output
Specifying `-v` to the pass will output the filename and linenumber in the
original C/C++ file where each occurance occured. The output is of the form:
`<filename>\t<linenumber>\n`. 

This requires that the bitcode was compiled with debugging metadata (`-g` to
clang). 

Example:
`````
opt -analyze -load $LLVM_LIB_DIR/mutate_PosixCondSignal.so -PosixCondSignal -v <test.bc >/dev/null
`````

#### Remove Mode
The `-rmmode` flag to the pass indicates that occurances should be remove. The
`-pos` flag accepts a comma seperated list of unsigned integers that should be
removed. `-pos` is indexed from 0 and corresponds to occurances that are
enumerated in find or find verbose. 

Example:
`````
# remove occurrance 0 and 1
opt -load $LLVM_LIB_DIR/mutate_PosixCondSignal.so -PosixCondSignal -rmmode -pos=0,1 <test.bc >out.bc
`````

Note that `-analyze` cannot be used in remove mode because the resulting
bitcode file will not be sent to `stdout`. 

#### Replace Mode
Replace mode swaps a call to `pthread_cond_signal` with
`pthread_cond_broadcast` and vice versa. 

Replace mode works similarly than remove. Use `-repmode` to specify that the
program should be in replace mode. Use `-pos` to specify positions to replace.

### Warnings
* The `-pos` input is not sanitized for duplicate entries. Specifying the same
position twice will result in a runtime error. This was done so that the entire
position list would not have to be iterated over. 

* If both `-rmmode` and `-repmode` are both specified `-rmmode` takes
  precedence. 
