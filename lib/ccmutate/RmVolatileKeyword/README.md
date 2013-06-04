## Readme
Author: Markus Kusano

Date: 2013-01-30

### Description
opt pass to remove the `volatile` keyword from certain instructions. Although
the `volatile` keyword in C/C++ is not used in the same way as in Java, it
still can be used for cross thread synchronization. One example would be reason
Peterson's algorithm style software level locks.

### Usage

#### Analyze
Passing the `-analyze` switch to opt causes the program to print out
information about occurrances of `volatile` instructions.

Consider the following C program (`test.c`)

`````
1.  int main(int argc, char *argv[]) {
2.	volatile int num1;
3.
4.	num1 = argc;
5.
6.	int num2 = num1;
7.
8.	num1 = argc * 37;
9.
10.	return 0;
11.  }
`````

And the LLVM bitcode disassembly 

`````
define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca i8**, align 4
  %num1 = alloca i32, align 4
  %num2 = alloca i32, align 4
  store i32 0, i32* %retval
  store i32 %argc, i32* %argc.addr, align 4
  call void @llvm.dbg.declare(metadata !{i32* %argc.addr}, metadata !13), !dbg !14
  store i8** %argv, i8*** %argv.addr, align 4
  call void @llvm.dbg.declare(metadata !{i8*** %argv.addr}, metadata !15), !dbg !14
  call void @llvm.dbg.declare(metadata !{i32* %num1}, metadata !16), !dbg !19
  %0 = load i32* %argc.addr, align 4, !dbg !20
  store volatile i32 %0, i32* %num1, align 4, !dbg !20
  call void @llvm.dbg.declare(metadata !{i32* %num2}, metadata !21), !dbg !22
  %1 = load volatile i32* %num1, align 4, !dbg !22
  store i32 %1, i32* %num2, align 4, !dbg !22
  %2 = load i32* %argc.addr, align 4, !dbg !23
  %mul = mul nsw i32 %2, 37, !dbg !23
  store volatile i32 %mul, i32* %num1, align 4, !dbg !23
  ret i32 0, !dbg !24
}
`````

Running the tool with no command line options

`````
opt -analyze -debug -load $LLVM_LIB_DIR/lib/RmVolatileKeyword.so -RmVolatileKeyword <test.bc >/dev/null
`````

Will simply output 3, the number of instructions marked as `volatile`.

#### Analyze Verbose
With the `-v` switch to the pass, debugging information is pulled from the bitcode and used
to provide the line number and filename of the original C/C++ code that
corresponds to where the `volatile` LLVM instruction occurs. The output is of the
form `<filename>\t<linenumber>\n` for each occurrance.

`-v` has no current effect when removing occurrences of `volatile`
instructions.

Note, this requires the bitcode to be compiled with debugging information (pass
`-g` to clang). A warning will be produced in verbose mode for each instruction
that has no debugging information associated.

It is important to note that this is not the location in C/C++ code where the
`volatile` keyword is used. It is the line of C/C++ code that compiled down to an
occurrence of the `volatile` keyword.

For example, considering the C and LLVM bitcode disassembly above:

Run the pass with `-v`:
`````
opt -analyze -debug -load $LLVM_LIB_DIR/lib/RmVolatileKeyword.so -RmVolatileKeyword -v <test.bc >/dev/null
`````

Which produces:
`````
test.c	4
test.c	6
test.c	8

`````

None of the line mentioned are the actual declaration of the volatile keyword,
but rather where the `integer` declared `volatile` was used.

#### rmpos
The switch `rmpos` is used to remove specific occurances of volatile
instructions. This is meant to be used in conjunction with the analyze parts of
the pass. 

For example, running the pass on the same `test.bc` file above:

`````
opt -debug -load $LLVM_LIB_DIR/lib/RmVolatileKeyword.so -RmVolatileKeyword -rmpos=2 <test.bc >out.bc
`````

Produces a file `out.bc` with the 3rd occurance of the volatile keyword remove
(the positions are zero indexed). 

More than one position can be removed: 
`````
opt -debug -load $LLVM_LIB_DIR/lib/RmVolatileKeyword.so -RmVolatileKeyword -rmpos=0,2 <test.bc >out.bc
`````

Produces a file `out.bc` with the 1st and 2nd occurance removed. 

Specifically, `-rmpos` accepts a comma seperated list of integers to remove.
Alternatively, each occurance can be specified with its own switch. For
example, `-rmpos=0,2` is equivalent to `-rmpos=0 -rmpos=2`.

The `-analyze` switch is still perfectly valid to use with `-rmpos`. 

## Design Motivation
The input LLVM bitcode file is never modified. 
