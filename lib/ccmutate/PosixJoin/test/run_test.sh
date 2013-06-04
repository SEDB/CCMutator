# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	%llvmdis: locatino of llvm-dis (for human readable output bitcode files)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_PosixJoin.so"
libraryName="-PosixJoin"

echo "BEGIN TEST: Non bitcode input file"
$opt -analyze -debug -load $llvmlibdir/$testLibName $libraryName <non_bitcode_file.bc >/dev/null
echo "END TEST: Non bitcode input file"
echo " "

echo "BEGIN TEST: Non bitcode input file"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName <non_bitcode_file.bc >/dev/null
echo "END TEST: Non bitcode input file"
echo " "

echo "BEGIN TEST: Find non verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -v <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Remove position 0, output to out01.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmmode -pos=0 <test.bc >out01.bc
$llvmdis <out01.bc >out01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1, output to out10.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmmode -pos=1 <test.bc >out10.bc
$llvmdis <out10.bc >out10.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1 and 0, output to out11.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmmode -pos=1,0 <test.bc >out11.bc
$llvmdis <out11.bc >out11.ll
echo "END TEST"
echo " "

echo "Begin Test: Remove out of bounds position output to out_f001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmmode -pos=10000 <test.bc >out_f001.bc
$llvmdis <out_f001.bc >out_f001.ll
echo "END TEST"
echo " "

# This test currently fails
echo "BEGIN TEST: Remove position zero twice output to out01_0.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmmode -pos=0,0 <test.bc >out01_0.bc
$llvmdis <out01_0.bc >out01_0.bc
echo "END TEST"
echo " "

# This test currently fails
echo "BEGIN TEST: Replace default position 0 twice output to outrep01_0.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -repmode -pos 0,0 <test.bc >outrep01_0.bc
$llvmdis <outrep01_0.bc >outrep01_0.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace 0 with default sleep val and output to out_s001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -repmode -pos 0 <test.bc >out_s001.bc
$llvmdis <out_s001.bc >out_s001.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace 0 and 1 with default sleep val and output to out_s011.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -repmode -pos=1,0 <test.bc >out_s011.bc
$llvmdis <out_s011.bc >out_s011.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace 0 and 1 with 32 second sleep val and output to out_s011_0.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -sleepval 32 -repmode -pos=1,0 <test.bc >out_s011_0.bc
$llvmdis <out_s011_0.bc >out_s011_0.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace 0 and 1 with -200 second sleep val and output to out_s011_1.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -sleepval -200 -repmode -pos=1,0 <test.bc >out_s011_1.bc
$llvmdis <out_s011_1.bc >out_s011_1.ll
echo "END TEST"
echo " "
