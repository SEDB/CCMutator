# CCMutator
Author: Markus Kusano `<mukusano at vt dot edu>`

## Requirements
LLVM source code and object files (tested using LLVM-3.2)

The source is released under the University of Illinois/NCSA public license
(free, non-viral/copyleft).

## Description
CCMutator is a set of partial and higher order mutation operators implemented
as `opt` passes on LLVM IR. Partial mutation operators allows for some subset of
mutations to occur. For example, instead of removing all calls to
`pthread_join` or creating all combinations or mutations the user can specify
which occurances to remove.

The passes do not modify the input LLVM bitcode file, this allows for easy
process level concurrency to be obtained using the UNIX `&` operator. All
possible combinations of mutations can still be created potentially with each
`opt` pass running as it's own process. 

### Operators
Most operators work similarly for both C++11 and POSIX (PThread) libraries.

#### Condition Variables
1. Remove call to `cond_wait()`
1. Remove call to `cond_timed_wait()`
1. Modify wait time for `cond_timed_wait()`
1. Switch `cond_timed_wait()` to `cond_wait`
1. Remove call to `cond_signal()` (POSIX only)
1. Replace call to `cond_signal()` with `cond_broadcast()` (POSIX only)
1. Replace call to `cond_broadcast()` with `cond_signal()` (POSIX only)

#### Mutexes
1. Remove lock-unlock pair
1. Swap lock-unlock pairs
1. Shrink critical section
1. Expand critical section
1. Split critical section 

#### Semaphores
1. Modify permit count on semaphore (POSIX only)

#### Yield
1. Remove call to `posix_yield()` or `sched_yield()`

#### Volatile Keyword
1. Remove volatile keyword from LLVM instructions

#### Thread Join
1. Remove call to `join()`
1. Replace call to `join()` with `sleep()`

#### Atomic Objects
All atomic operators only applicable to C++11
1. Remove memory fence & n/a & \checkmark
1. Modify memory fence ordering constraint
1. Replace single-thread sync fence with cross-thread
1. Replace cross-thread sync fence with single-thread
1. Replace atomic load with non-atomic load
1. Modify atomic load ordering constraint
1. Replace single-thread sync atomic load with cross-thread
1. Replace cross-thread sync atomic load with single-thread
1. Replace atomic store with non-atomic store
1. Modify atomic store ordering constraint
1. Replace single-thread sync atomic store with cross-thread
1. Replace cross-thread sync atomic store with single-thread
1. Modify atomic read-modify-write ordering constraint
1. Replace single-thread sync atomic read-modify-write with cross-thread
1. Replace cross-thread sync atomic read-modify-write with single-thread
1. Modify compare exchange ordering constraint
1. Replace single-thread sync compare exchange with cross-thread
1. Replace cross-thread sync compare exchange with single-thread

## Installation
A `configure` script is provided that at least needs to have the following passed to it:

1. `--with-llvmsrc <Location of LLVM Source Code>`
1. `--with-llvmobj <Location of LLVM Object Code>`

For example:

`./configure --with-llvmsrc=/home/markus/src/llvm-3.2.src/ --with-llvmobj=/home/markus/src/install-3.2/`

`./configure --help` will list other available options.

Then simply run `make` and `make install`.

This creates a set of library files in `<installDir>/lib`. 

## Usage
The result of installation is a set of library files that can be loaded by
`LLVM`'s `opt` using the `-load` flag.

`./lib/ccmutate` contains the source files and `README`s for each mutation
operator. See these `README`s for information on using each individual operator.
The operators also have test scripts that show examples of their use.
