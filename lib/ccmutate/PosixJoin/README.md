## Readme
Author: Markus Kusano

### Description
Mutation operators modifying calls to `pthread_join`. Calls to `pthread_join`
can either be removed or replaced with a call to sleep. 

The pass works in two different modes: find or remove/replace.

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

### Possible Improvments
Use `usleep()` instead of sleep for more fine grained mutation. 
