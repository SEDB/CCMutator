# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	%llvmdis: locatino of llvm-dis (for human readable output bitcode files)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_PosixCondSignal.so"
libraryName="PosixCondSignal"

echo "BEGIN TEST: Non bitcode input file"
$opt -analyze -debug -load $llvmlibdir/$testLibName -PosixCondSignal <non_bitcode_file.bc >/dev/null
echo "END TEST: Non bitcode input file"
echo " "

echo "BEGIN TEST: Non bitcode input file"
$opt -analyze -debug -load $llvmlibdir/$testLibName -PosixCondSignal <non_bitcode_file.bc >/dev/null
echo "END TEST: Non bitcode input file"
echo " "

echo "BEGIN TEST: Find non verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -PosixCondSignal <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -PosixCondSignal -v <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Remove position 0, output to out01.bc"
$opt -debug -load $llvmlibdir/$testLibName -PosixCondSignal -rmmode -pos=0 <test.bc >out01.bc
$llvmdis <out01.bc >out01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1, output to out10.bc"
$opt -debug -load $llvmlibdir/$testLibName -PosixCondSignal -rmmode -pos=1 <test.bc >out10.bc
$llvmdis <out10.bc >out10.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1 and 0, output to out11.bc"
$opt -debug -load $llvmlibdir/$testLibName -PosixCondSignal -rmmode -pos=1,0 <test.bc >out11.bc
$llvmdis <out11.bc >out11.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position zero twice"
$opt -debug -load $llvmlibdir/$testLibName -PosixCondSignal -rmmode -pos=0,0 <test.bc >out11.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove out of bounds position output to out_f001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmmode -pos=10000 <test.bc >out_f001.bc
$llvmdis <out_f001.bc >out_f001.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Replace position 0 twice out of bounds position output to out_r01.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -repmode -pos=0,0 <test.bc >out_r01.bc
$llvmdis <out_r01.bc >out_r01.ll
echo "END TEST"
echo " "
