## Mutaiton Operators for Fence

### Description
Removes occurrences of fence instructions

### Usage
See `test/run_test.sh` for actual usage examples.

With no flags, the output is the number of occurrences of fence instructions
found in the LLVM input file. For example:

`````
opt -analyze -load $CCMUTATE_LIB/$testLibName -Fence <test.bc >/dev/null
`````

#### Verbose
With the `-verbose` flag, the output is in the following form (with escaped
characters):
`````
<index number>\t<source filename>:<source linenumer>
\t<fence instruction>\n
`````
for each instruction found

Example:
`````
opt -analyze -load $CCMUTATE_LIB/$testLibName -Fence -verbose <test.bc
>/dev/null
`````

#### -rm
The flag `-rm` will remove fence instructions at indecies specified with
`-pos`. `-pos` is zero index and corresponds to outputs from `-verbose` or no
argument passes.

Example:
`````
opt -load $CCMUTATE_LIB/$testLibName -Fence -rm -pos=0 <test.bc >out_001.bc
`````

Note that `-analyze` is no longer used.

#### -mod
The flag `-mod` will change the atomic ordering to a value specified with
`-order`. `-order` must be specified. `-order` takes a list of unsigned
integers that correspond to the desired atomic ordering value for each
position specified with `-pos`. i.e. `-pos=0,1,3 -order=2,3,1` will set
position 0 to ordering 2, position 1 to ordering 3 and position 3 to ordering
1.

The values of `-order` correspond as follows:

0. acquire
1. release
2. acquire release
3. sequential consistency

See http://llvm.org/docs/LangRef.html#ordering for information on what
guarantees the orderings provide.

If `-order` is shorter than `-pos` then the last value specified in `-order`
will be used for the remaining indecies. This will let you easily change a
group of fence instructions to the same atomic ordering. e.g. `-pos=0,1,3
-order=1` will change all three positions to release semantics.

Example:
`````
opt -load $CCMUTATE_LIB/$testLibName -Fence -mod -order=1 -pos=0  <test.bc >out_002.bc
`````
This will modify position 0 to release semantics.

#### -scope
`-scope` will toggle the syncrhonization scope for the positions specified with
`-pos`. This change a singlethreaded fence to multi-threaded and vice-versa.

Example:
`````
opt -load $CCMUTATE_LIB/$testLibName -Fence -scope -pos=1 <test.bc >out_004.bc
`````
This will toggle position 1's synchronization scope.

### Relevance
Examined C++11 code using `std::atomic_thread_fence()` compiles down to LLVM
`fence` instructions.
