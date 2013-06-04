## Readme Posix Cond Wait
Author: Markus Kusano

Date: 2013-02-28

### Description
Mutator for calls to `pthread_cond_wait()` and `pthread_cond_timedwait()`. The
desired mutation operators are:
    1. Remove call to `pthread_cond_wait()` or `pthread_cond_timed_wait()`
    1. Modify parameter of `pthread_cond_wait()` or `pthread_cond_timed_wait()`

For modifying the paremeter, this could be changing the mutex, condition or
time to wait. 

### Removing call 
Specify that the tool should remove occurrences with the `-rm` flag. Specify
positions to remove with `-pos`. `-pos` accepts a comma separated list of
values to modify (0 indexed). For example:

`````
opt -debug -load $llvmlibdir/mutate_PosixCondWait.so -PosixCondWait -rm -pos=1,0 <test.bc
>out.bc
`````

This will remove occurrence 1 and 0 and output the resulting bitcode to
`out.bc`. 

If the return value of the call is used elsewhere, the call is not removed but
replaces with `0`, ie `%ret = call pthread_cond_wait...` becomes `%ret = 0`.

### Mutating `pthread_cond_timedwait()`
Currently the operator lets you add or subtract values from the associated call
to `pthread_cond_timedwait()`.

The modifications are performed right before the call to
`pthread_cond_timedwait()`. The modifications do not create a new `struct
timespec` but simply modify the one being used in the call. This could lead to
mutations occurring on multiple locations if they share the same `timespec`
stuct. 

The modification values are specified by two command line options that accept
lists of integers: `-nsecval` and `-secval`. These specify the ammount to add
to the `timespec`'s `tv_nsec` and `tv_sec` values. Any valid integer is
accepted (so negative values can be used) 

Each value in `-nsecval` and `-secval` corresponds to a position in `-pos`. For
example, specifying `-pos=1,0,7` and `-secval=0,-1,5` will add `0` to position
`1`, `-1` to position `0` and `5` to position `7`. `-nsecval` works similarly.

If the length of either `nsecval` or `secval` is shorter than `pos` then the
last value specified will be used for the reminaing positions. For example, if
`-pos=3,1,0` and `-secval=1` then `1` will be added to positions `3`, `1` and
`0`. Again, `nsecval` works similarly.

`secval` and `nsecval` need not be the same size, the only requirment is that
you specify atleast 1 item in both.

The mutation is performed as an insertion of an addition to the `tv_sec` and
`tv_nsec` elements of the timespec struct. By default, the instructions are
inserted right before the call to `pthread_cond_timedwait()`. Because
`pthread_cond_timedwait()` is often called within a `while` loop, this could
lead to multiple mutations of the `timespec` structure. 

An additional argument, `-inspt` accepts arguments similar to `-secval` and
`-nsecval`. The input is a comma separated list of integers, each position
corresponds to a matching position in `-pos` and if there is no value in
`-inspt` specified for a given position then the last value in the list is
used. The value represents the number of instructions from the start of the
function where the mutation code should be added. Only positive values are
valid.

Be careful when using `-inspt`; no error checking is performed on if the
mutation instructions will generate a runtime error (like a seg fault). The
mutation code involves `load`s and `store`s to locations in memory.

It could also lead to runtime errors by the `opt` verification pass that,
`Instruction does not dominate all uses!`. This is caused by the instruction
attempting to use a value that doesn't exist yet. This could occur when the
mutation code is set to occur before the `timespec` parameter in the call to
`pthread_cond_timedwait` is created.

Putting it all together in an example:

`````
opt -debug -load $llvmlibdir/$testLibName -$libraryName -inspt=24 -secval=-1 -nsecval=2000 -tmod -pos=0 <test.bc >out.bc
`````

The above will place mutation code before the 24th instruction in the function
containing the 0th occurrence of `pthread_cond_{,timed}wait()`. The
`timespec`'s second value will be decreased by 1 and the nano second value will
be increased by 2000.

Currently, modifying the time value of a call to `pthread_cond_wait()` (i.e.
converting it into a call to `pthread_cond_timedwait()` is not implemented.

### Concerns
`timspec` is a somewhat opaque type. POSIX seems to only require that it
atleast has two fields:

    1. `time_t  tv_sec`
    1. `long    tv_nsec`

Representing seconds and nanoseconds respectively. This raises two questions:
what is `time_t` and which element of the struct is `tv_sec` and which element
is `tv_nsec`. 

On my system, examining the resulting LLVM bitcode, the type definition is:

`````
%struct.timespec = type { i32, i32 }
`````

The two `i32`s refer to `timespec.tv_sec` and `timespec.tv_nsec` respectively.
Reading the POSIX documentation leads me to believe that there is no guarantee
that these positions be constant across platforms. Two globals in
`PosixCondWait.cpp` refer to the indecies of these positions and can be
modified at compile time (see `tv_sec_loc` and `tv_nsec_loc`).
