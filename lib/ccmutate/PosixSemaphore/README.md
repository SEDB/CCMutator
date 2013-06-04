## Readme
Author: Markus Kusano

Date: 2013-02-10

### Description
Mutations operators that work on POSIX semaphores. 

Currently, the following operation is implemented:

* Change permit  count on semaphore 

### Implementation
Two function calls are examined by the pass:

`````
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
int sem_init(sem_t *sem, int pshared, unsigned int value);
`````

The `value` parameter of both functions is modified. For `sem_open` the `value`
parameter is only availible when `O_CREAT` is passed as the flag, so only those
calls that have the parameter are modified.

This will only sometimes modify the `value` of a semaphore because if `O_CREAT`
is specified and the semaphore already exists then the `mode` and `value` flags
are ignored.

### Usage
The pass has two modes, find and modify. By default the pass runs in find mode
with non verbose output. 

#### Find Mode
Example:
`````
opt -analyze -load $LLVM_LIB_LOCATION/mutate_PosixSemaphore.so -PosixSema <test.bc >/dev/null
`````

This will output the number of occurrences of function calls that set or modify
the permit count of a semaphore. 

Verbose mode is activated with `-v`. It requires that the bitcode is compiled
with debugging metadata (`-g` to clang). The output is the filename and line
number of the occurrence of each permit count modifying function call. The
output is of the form: `<filename>\t<linenumber>\n`. 

Example:
`````
opt -analyze -load $LLVM_LIB_LOCATION/mutate_PosixSemaphore.so -PosixSema -v <test.bc >/dev/null
`````

Note: for both modes, the `-analyze` argument is required for the output to be
printed. 

#### Modify Mode
Modify mode is specified by the flag `-mod` and allows the permit count of any
combination of occurrences of semaphore permit value call instructions to have
their value modified. 

Modify mode accepts two as arguments: one for the positions to modify (`-pos`) and
another for the value to set each respective occurrence. 

For example, `-pos=0,2,4 -val=37,1,19` will set occurrence `0` to `37`,
occurrence `2` to `1` and occurrence `4` to `19`. Values must be less than
`SEM_VALUE_MAX`. 

If a respective `-val` is not specified for a `-pos` then `rand()` seeded with
the system time is used to set the value to a pseudo-random number.
