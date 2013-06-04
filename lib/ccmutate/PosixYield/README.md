## POSIX Yield Mutation Operators 
Markus Kusano

2013-02-19

### Description
Mutation operator that removes occurances of either `posix_yield()` or `sched_yield()`.

### Usage
The most basic usage of the tool is to run it with simply an input file name:

`````
mutator-posix-yield test.bc
`````

This runs the tool in its default "find" mode. The output is the number of
occurrences of `{sched,posix}_yield()`that are found followed by a newline
character to stdout. This is meant to be used in conjunction with the other
modes of operation

#### -rm
`-rm` specifies that the tool should remove specific occurrences of
`{sched,posix}_yield()`. This can be any combination desired. 

Occurrences are specified using `-pos` which accepts a list. This list is the
index of the occurrence to remove. 

For example, if the default find mode (see above) returned 5 then any
combination of the numbers 0-5 could be put in `-pos`. The result would be an
output file that has these occurrences removed. The output file can be
specified with `-o` and defaults to `out.bc`. `-pos` accepts a comma separated
list of values or 1 value.

Example:
`````
mutator-posix-yield -rm -pos=1,2,7 -o mutated_test.bc test.bc
`````

Which is equivalent to:
`````
mutator-posix-yield -rm -pos 1 -pos 2 -pos 7 -o mutated_test.bc test.bc
`````

#### -verbose
`-verbose` alters the operation of the default find mode and thus only makes
sense if run in find mode (that is, `-rm` is not specified). 

With `-verbose` the output contains information of the filename and linenumber
of each occurrence with respect to the _original_ source file.

This is implemented using the debugger information provided in the LLVM bitcode
file. Because of this, `-verbose` will only provide useful information if the
file is built with debug metadata. This can be done with `-g` passed to
`clang`. Whatever filename/linenumber information that is provided by the
compile that created the LLVM bitcode is used.

The information is formatted as: 

`````
<filename>\t<linenumber>\n
`````

For each found occurrence. A warning is displayed if no debugger information is
found.

### Limitations/Todo
Currently, on my system, calls to `pthread_yield()` are indirect. No alias
analysis is done with this tool so there is no way to determine if the indirect
function pointer is pointing to the correct call. This will hopefully be fixed
in the future.
