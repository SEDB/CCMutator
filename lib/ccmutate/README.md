## Readme
Author: Markus Kusano

### Description
Set of partial mutation operators implemented as opt passes on LLVM IR. Partial
mutation operators allows for some subset of mutations to occur. For example,
instead of removing all calls to `pthread_join` or creating all combinations or
mutations the user can specify which occurances to remove.

The passes do not modify the input LLVM bitcode file, this allows for easy
process level concurrency to be obtained using the UNIX `&` operator. All
possible combinations of mutations can still be created potentially with each
`opt` pass running as it's own process. 

## Operators
* PosixJoin: Find and remove or replace with a call to sleep() calls to
  `pthread_join`

* RmVolatileKeyword: Removes the volatile keyword from some LLVM IR
  instructions


### C++11 Specific ABI Information
Clang/LLVM does not currently (2013-03-19) appear to supply a C++ function name
demangler. 

`libstdc++` (GCC), however, does. This is used by the pass when it is told to
mutate C++11's `std::thread.join`. 

Hopefully there are no differences between the implementations of the generic
Itanium ABI between Clang and `libstdc++`.

