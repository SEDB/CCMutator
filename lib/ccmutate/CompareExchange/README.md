## Compare Exchange Mutation Operator

### Description
Mutation operator to modify the concurrency aspects of atomic compare exchange
instructions.

### Usage
See `test/run_test.sh` for more examples.

With no arguments, the operator outputs the number of `cmpxchg` instructions
found.

Example:
`````
opt -analyze -load $CCMUTATE_LIB/$testLibName -CmpXchg <test.bc >/dev/null
`````

#### -verbose: Verbose Output
Verbose mode displays the source filename, linenumber and instruction for each
occurrence found.

`````
opt -analyze -load $CCMUTATE_LIB/$testLibName -CmpXchg -verbose <test.bc >/dev/null
`````

The output is of the form (with escaped characters):
`````
<index number>\t<source filename>:<source linenumer>
\t<cmpxchg instruction>\n
`````
For each atomic store instruction found.

#### -mod: Modify Atomic Ordering Constraint
The flag `-mod` will change the atomic ordering to a value specified with
`-order`. `-order` must be specified. `-order` takes a list of unsigned
integers that correspond to the desired atomic ordering value for each
position specified with `-pos`. i.e. `-pos=0,1,3 -order=2,3,1` will set
position 0 to ordering 2, position 1 to ordering 3 and position 3 to ordering
1.

The values of `-order` correspond as follows:

0. monotonic
1. acquire
2. release
3. acquire release
4. sequentially consistent

See http://llvm.org/docs/LangRef.html#ordering for information on what
guarantees the orderings provide.

If `-order` is shorter than `-pos` then the last value specified in `-order`
will be used for the remaining indecies. This will let you easily change a
group of fence instructions to the same atomic ordering. e.g. `-pos=0,1,3
-order=1` will change all three positions to acquire semantics

Example:
`````
opt -load $CCMUTATE_LIB/$testLibName -CmpXchg -mod -pos=0,1,2 -order=0,3 <test.bc >out_006.bc
`````
This will change position 0 to monotonic, position 1 to acquire release
and position 2 to acquire release.

#### -scope: Change Synchronization Scope
`-scope` will toggle the syncrhonization scope for the positions specified with
`-pos`. This change a single-threaded `store` to multi-threaded and vice-versa.

Example:
`````
opt -load $CCMUTATE_LIB/$testLibName -CmpXchg -scope -pos=0 <test.bc >out_007.bc
`````

This will toggle position 0's synchronization scope.

### Future Work
Toggle `cmpxchg` instruction to non-atomic version of the same operation.

### Relevance
Examining clang produced output of C++ files using functions like
`std::atomic::compare_exchange_strong()` show that `cmpxchg` instructions are
produced.
