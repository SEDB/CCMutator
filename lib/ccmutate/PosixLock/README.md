## Readme RmLockPairs
Author: Markus Kusano

### Description
Pass to remove pair of calls to `pthread_mutex_lock()` and
`pthread_mutex_unlock()`.

### Operation
Passing the `-analyze` flag to opt and giving the pass no command line options
will cause the tool to output information about the calls it has encountered. 

Specifically, it will display the number of functions found with calls to
`pthread_mutex_lock()` and/or `pthread_mutex_unlock()` on one line and on the
next it will display the number of pairs of calls to `pthread_mutex_lock()` or
`pthread_mutex_unlock()` found in each function.

#### -verbose
The flag `-verbose` will enable verbose output. The output has the following
general form:

`````
<functionName>
    <llvm IR call to pthread_mutex_lock>
    <llvm IR call to pthread_mutex_unlock>
    <C source code filename of function>    <C source code line of lock>    <C
    source code line of unlock>	<distance between lock and unlock call>
    ... ... ...
`````

This form is repeated for each lock/unlock pair found and each function found
to have calls to pthread lock or unlock.

It is possible for a function to have 0 pairs: this means that it has calls to
`pthread_mutex_lock` or `pthread_mutex_unlock` but none of the calls were found
to have aliased to the same mutex.

Example:

`````
opt -basicaa -analyze -debug "$llvmlibdir"/"mutate_PosixLock" -PosixLock -verbose <test_local.bc >/dev/null
`````

#### -rm
The flag `-rm` will specify that the tool is to remove lock/unlock pairs. The
pairs to remove are specified with `-pos`. 

The pairs themselves have two indices: a function index and pair index. Both of
these should be specified to `-pos` as two values. These values can be obtained
by running the tool with `-verbose` or with no command line flags. 

Example:
`````
opt -basicaa -load "$llvmlibdir"/"mutate_PosixLock" -PosixLock -rm -pos=0,0 -pos=0,1 <test_local.bc >out_01.bc
`````

The two `-pos` options specified in the above example are equivalent to writing
`-pos=0,0,0,1`. Because the positions must be specified as pairs, it is
required that the number of items specified to `-pos` be even.

Pairs specified may share a common call to `pthread_mutex_lock()`. For example,
consider the following:

`````
pthread_mutex_lock(mut);
if (cond) {
    pthread_mutex_unlock(mut);
    return -1;
}
else {
    pthread_mutex_unlock(&mut);
    return 0;
}
`````

In the above example, the same call to `pthread_mutex_lock()` pairs with two
calls to `pthread_mutex_unlock()`. If both pairs are specified then three calls
get removed. However, if only one pair is specified, the call to
`pthread_mutex_unlock()` in the other pair will still remain, leading to a
situation where a call to `pthread_mutex_unlock()` does not occur after a call
to `pthread_mutex_lock()` on the same mutex. 

This could lead to undefined behavior, but allows for a bug to be inserted
(call to unlock without lock) so it is left in (read: it's a feature not a
bug).

#### -swap
The flag `-swap` allows for two lock-unlock pairs to be swapped. Similar to
`-rm`, `-pos` is again used to specify which pairs to remove. It is required
that `-pos` have groups of four integers (two pairs of two) specified.

Note: similar to `-rm`, the same lock call can be found in multiple lock-unlock
pairs. This means it is possible to switch two pairs that are essentially the
same. The tool will issue a wanring and not perform the operation if this is
attempted.

Example: 
`````
opt -basicaa -debug -load "$llvmlibdir"/mutate_PosixLock -PosixLock -swap -pos=0,0,0,2 <test_local.bc >out_02.bc
`````

The above will swap the lock-unlock pair at position (0,0) with the lock-unlock
pair at position (0,2).

Note: the swaps are performed consecutively, so if you swap the same pair twice
the result is essentially a no-op.

#### -shift
`-shift` allows for the lock and/or unlock call of a pair to be moved in
arbitrary directions. Similar to the other mutations, it uses `-pos` to specify
function and lock-unlock pairs that are desired to be mutated.

Two new list command line options, `-lockdir` and `-unlockdir` are used to
specify the direction for the shifts. Each element in `-lockdir` and
`-unlockdir` corresponds to the direction to shift the lock and unlock call of
each pair specified in `-pos`. 

Shift directions are either positive or negative. Positive shifts downwards
(towards a high line number) while negative shifts upwards (towards a lower
line number).

For example: 

Note: the shifts are performed consecutively. The user needs to account for
another lock call being moved in the path of another shift and add an
additional direction unit to reach the desired position.

`````
opt -basicaa -debug -load "$llvmlibdir"/mutate_PosixLock -PosixLock -shift -pos=0,0 -pos=0,2 -lockdir=2,-2 -unlockdir=2,-2 <test_local.bc >out_08.bc
`````

The above will shift the lock call of position (0,0) down two places and the
unlock call down two places. It will also shift the lock call of pair (0,2) up
two positions and the unlock call up two positions.

To be precise, the `-lockdir` and `-unlockdir` values specifies the instruction
where the lock/unlock call should be inserted before.

For example, consider the following snippet of LLVM IR:

`````
%call1 = call i32 @pthread_mutex_init(%union.pthread_mutex_t* %mut2, %union.pthread_mutexattr_t* null) nounwind
%call2 = call i32 @pthread_mutex_lock(%union.pthread_mutex_t* %mut1) nounwind
call void @llvm.dbg.declare(metadata !{i32* %a}, metadata !52)
%0 = load i32* %argc.addr, align 4
store i32 %0, i32* %a, align 4
`````

Specifying a `-lockdir` of 1 for the call to `pthread_mutex_lock` says that it
should be inserted before the call to `llvm.dbg.declare`, which moves it
nowhere.

A warning is output for this case.

If no shift direction is specified then the default is a shift of zero, a
warning is output in this case.

#### -split
`-split` allows for a lock-unlock pair to be split at an arbitrary position.

It takes a pair of positions relative to the lock call in the pair to insert
the new unlock and lock call. These are specified with `-splitpos`.

For example: 

`````
opt -basicaa -debug -load "$llvmlibdir"/mutate_PosixLock -PosixLock -split -pos=0,0 -splitpos=3,5 <test_local.bc >out_09.bc
`````

Will insert a call to unlock before the instruction 3 instructions after the
lock call of the pair and a call to unlock 5 instructions after the lock call
of the pair.

For example the following LLVM IR:
`````
  %call1 = call i32 @pthread_mutex_init(%union.pthread_mutex_t* %mut2, %union.pthread_mutexattr_t* null) nounwind, !dbg !50
  %call2 = call i32 @pthread_mutex_lock(%union.pthread_mutex_t* %mut1) nounwind, !dbg !51
  call void @llvm.dbg.declare(metadata !{i32* %a}, metadata !52), !dbg !53
  %0 = load i32* %argc.addr, align 4, !dbg !53
  store i32 %0, i32* %a, align 4, !dbg !53
  %call3 = call i32 @rand() nounwind, !dbg !54
  %rem = srem i32 %call3, 2, !dbg !54
  %tobool = icmp ne i32 %rem, 0, !dbg !54
  br i1 %tobool, label %if.then, label %if.end, !dbg !54

if.then:                                          ; preds = %entry
  %call4 = call i32 @pthread_mutex_unlock(%union.pthread_mutex_t* %mut1) nounwind, !dbg !55
`````

Becomes the following after the mutation (the inserted call are stored in
`mut_unlockSplit` and `mut_lockSplit` registers):

`````
  %call2 = call i32 @pthread_mutex_lock(%union.pthread_mutex_t* %mut1) nounwind, !dbg !51
  call void @llvm.dbg.declare(metadata !{i32* %a}, metadata !52), !dbg !53
  %0 = load i32* %argc.addr, align 4, !dbg !53
  %mut_unlockSplit = call i32 @pthread_mutex_unlock(%union.pthread_mutex_t* %mut1)
  store i32 %0, i32* %a, align 4, !dbg !53
  %call3 = call i32 @rand() nounwind, !dbg !54
  %mut_lockSplit = call i32 @pthread_mutex_lock(%union.pthread_mutex_t* %mut1)
  %rem = srem i32 %call3, 2, !dbg !54
  %tobool = icmp ne i32 %rem, 0, !dbg !54
  br i1 %tobool, label %if.then, label %if.end, !dbg !54

if.then:                                          ; preds = %entry
  %call4 = call i32 @pthread_mutex_unlock(%union.pthread_mutex_t* %mut1) nounwind, !dbg !55
`````

`-splitpos` must be specified in pairs for each pair specified in `-pos`.

Note: it is considered valid for the unlock position to be greater than the
lock position, it just should be noted that it results in a mutation that looks
like:

`````
pthread_mutex_lock(&mut);
...
pthread_mutex_lock(&mut);
....
pthread_mutex_unlock(&mut);
...
pthread_mutex_unlock(&mut);
`````

This could be useful in testing recursive mutex usage so it is included and no
warnings are issued when this is done.

### Limitations
Currently only lock unlock pairs local to the same function are able to be
mutated.

### Todo
Add ability to remove all pairs that are related to a certain call to
`pthread_mutex_lock()`.

Perhaps the shifting should be simplified so a shift of +1 is not a no-op
