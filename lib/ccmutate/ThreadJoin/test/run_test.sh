# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	%llvmdis: locatino of llvm-dis (for human readable output bitcode files)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_ThreadJoin.so"
libraryName="-ThreadJoin"

#echo "BEGIN TEST: Non bitcode input file"
#$opt -analyze -debug -load $llvmlibdir/$testLibName $libraryName <non_bitcode_file.bc >/dev/null
#echo "END TEST: Non bitcode input file"
#echo " "

#echo "BEGIN TEST: Non bitcode input file"
#$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName <non_bitcode_file.bc >/dev/null
#echo "END TEST: Non bitcode input file"
#echo " "

echo "BEGIN TEST: Find verbose no posix or C++11 (should fail)"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -v <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Find non verbose posix"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -posix <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find non verbose c++11"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -c++11 <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find non verbose c++11 and posix"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -c++11 -posix <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find verbose posix"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -posix -v <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Find verbose C++11"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -v -c++11 <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Find verbose C++11 and Posix"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -v -c++11 -posix <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: remove no positions specified C++11 and Posix"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm -c++11 -posix <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Remove position 0 posix and C++11, output to out01.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm  -c++11 -posix -pos=0 <test.bc >out01.bc
$llvmdis <out01.bc >out01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1, output to out02.bc (c++11 and posix)"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm -c++11 -posix -pos=1 <test.bc >out02.bc
$llvmdis <out02.bc >out02.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 3, output to out03.bc (c++11 and posix)"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm -c++11 -posix -pos=3 <test.bc >out03.bc
$llvmdis <out03.bc >out03.ll
echo "END TEST"
echo " "

echo "Begin Test: Remove out of bounds position (c++11 and posix) (should fail)"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -c++11 -posix -rm -pos=4 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position zero twice output to out01_0.bc (-c++11 and -posix)"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm -c++11 -posix -pos=0,0 <test.bc >out01_0.bc
$llvmdis <out01_0.bc >out_01_0.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace w/o -posix or -c++11"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rep -pos 0 <test.bc >out_s001.bc
$llvmdis <out_s001.bc >out_s001.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace 3 with default sleep val and output to out_s001.bc (c++11 and posix)"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -c++11 -posix -rep -pos 3 <test.bc >out_s001.bc
$llvmdis <out_s001.bc >out_s001.ll
echo "END TEST"
echo " "

#echo "BEGIN TEST: Replace 0 and 1 with default sleep val and output to out_s011.bc"
#$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rep -pos=1,0 <test.bc >out_s011.bc
#$llvmdis <out_s011.bc >out_s011.ll
#echo "END TEST"
#echo " "

echo "BEGIN TEST: Replace 0,1,3 with 1 second sleep val and output to out_s011_0.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -posix -c++11 -sleepval 1 -rep -pos=1,0,3 <test.bc >out_s011_0.bc
$llvmdis <out_s011_0.bc >out_s011_0.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace 0 and 1 with -200 second sleep val and output to out_s011_1.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -sleepval -200 -repmode -pos=1,0 <test.bc >out_s011_1.bc
$llvmdis <out_s011_1.bc >out_s011_1.ll
echo "END TEST"
echo " "
