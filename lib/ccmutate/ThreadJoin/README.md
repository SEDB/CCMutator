## Thread Join Readme
Author: Markus Kusano

### Description
Mutation operators for modifying calls to C/C++ `join` synchonrization calls.
The tool currently targets POSIX and C++11 join, `pthread_join()` and
`std::thread.join()` respectively.

The pass works in two different modes: find or remove/replace.

### Usage
First, the search space must be specified. This is done with `-posix` to search
for `pthread_join` and/or `-c++11` to search for `std::thread::join`. 

#### Find Mode
Find has both verbose (`-v`) and default outputs. In order for the results to
be printed, `opt` must be run with `-analyze`. 

In non-verbose mode, the output is simply the number of occurrences. In verbose
mode, the output is the original source file and line number of each ocurrance.

For example, find with verbose:
`````
opt -analyze -load ../../../../Debug+Asserts/lib/mutate_PosixJoin.so -FindPosixJoin -v <test.bc >/dev/null
`````

In verbose, the output is: `<filename>\t<linenumber>\n` for each occurrance. 

#### Remove/Replace Mode
The `-rmmode` flag specifies that occurrences of `pthread_join` specified by
`-pos` should be removed. This should be used in conjunction with find mode.

For example, the following would remove occurrences 0 and 2 and store the result
in `out.bc`
`````
opt-load ../../../../Debug+Asserts/lib/FindPosixJoin.so -FindPosixJoin -rmmode -pos=0,2 <test.bc >out.bc
`````

`-pos` accepts a comma seperated list of values or individual values. For
example `-pos=0,3,7` is equivalent to `-pos 0 -pos 3 -pos 7`.

Replace mode works similarly. It is enabled with `-repmode` and replaces
occurrences designated by `-pos`. 

By default, a call to `sleep(1)` (one second sleep) is inserted in place of the
call to `pthread_join`. The optional switch `-sleepval` can be used to specify
any value to be inserted. 

If the return value of the call to `pthread_join()` is used then it is replaced
with the return value of the call to `sleep()` that replaces it. Since
`std::thread::join()` has no return value, this does not apply to C++11
mutations.

### Limitations
Currently, the function call searched for in C++11 mode is
`std::__1::thread::join`. This appears to be what Clang uses when linked
against C++11. I could not get Clang to compile against libstdc++ due a bug in
the library. The tool will probably not work with code compiled with libstdc++.

Primairly in C++ code the LLVM `invoke` instruction is often used which cannot
be as easily removed or replaced since it is a terminator instruction. The
current solution is to always replace an `invoke` instruction with a `branch`
instruction to the normal destination of the `invoke` instruction.
