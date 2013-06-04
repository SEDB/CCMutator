# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	$llvmdis: locatino of llvm-dis (for human readable output bitcode files)
#	$llc: location of llc (static LLVM compiler)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_RmVolatileKeyword.so"
libraryName="RmVolatileKeyword"

echo "BEGIN TEST: Find non verbose"
$opt -analyze -debug -load "$llvmlibdir"/"$testLibName" -$libraryName <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -v <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Modify position 0, output to out01.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmpos=0 <test.bc >out01.bc
$llvmdis <out01.bc >out01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 1, output to out10.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmpos=1 <test.bc >out10.bc \
&& $llvmdis <out10.bc >out10.ll \
&& $llc out10.bc -o out10.s && gcc -lrt out10.s -o out10.exe && chmod +x out10.exe
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1 and 0, output to out11.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmpos=1,0 <test.bc >out11.bc
$llvmdis <out11.bc >out11.ll
echo "END TEST"
echo " "

echo "Begin Test: Modify out of bounds position output to out_f001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmpos=10000 <test.bc >out_f001.bc
$llvmdis <out_f001.bc >out_f001.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position zero twice output to out01_0.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmpos=0,0 <test.bc >out01_0.bc
$llvmdis <out01_0.bc >out01_0.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Specify verbose and rmpos"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rmpos -verbose <test.bc >out01_0.bc
echo "END TEST"
echo " "
