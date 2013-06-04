# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	$llvmdis: locatino of llvm-dis (for human readable output bitcode files)
#	$llc: location of llc (static LLVM compiler)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_PosixSemaphore.so"
libraryName="PosixSema"

echo "BEGIN TEST: Find non verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -v <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Modify position 0, output to out01.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -mod -pos=0 <test.bc >out01.bc
$llvmdis <out01.bc >out01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 1, output to out10.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -mod -pos=1 <test.bc >out10.bc \
&& $llvmdis <out10.bc >out10.ll \
&& $llc out10.bc -o out10.s && gcc -lrt out10.s -o out10.exe && chmod +x out10.exe
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1 and 0, output to out11.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -mod -pos=1,0 <test.bc >out11.bc
$llvmdis <out11.bc >out11.ll
echo "END TEST"
echo " "

echo "Begin Test: Modify out of bounds position output to out_f001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -mod -pos=10000 <test.bc >out_f001.bc
$llvmdis <out_f001.bc >out_f001.ll
echo "END TEST"
echo " "

# This test currently fails
echo "BEGIN TEST: Modify position zero twice output to out01_0.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -mod -pos=0,0 <test.bc >out01_0.bc
$llvmdis <out01_0.bc >out01_0.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0 to 34 output to out_0_34.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -mod -pos 0 -val 34 <test.bc >out_0_34.bc && $llvmdis <out_0_34.bc >out_0_34.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: modify position 0 to 2147483648 (2^32 or SEM_VALUE_MAX + 1 on my system)"
$opt -debug -load /home/markus/Repos/llvm-3.2.src/Release+Asserts/lib/mutate_PosixSemaphore.so -PosixSema -mod -pos 0 -val 2147483648 <test.bc >out.bc && $llvmdis <out.bc >out.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: modify position 0 and 1 with no value specified for 1 output to out_noval_001.bc"
$opt -debug -load /home/markus/Repos/llvm-3.2.src/Release+Asserts/lib/mutate_PosixSemaphore.so -PosixSema -mod -pos 0,1 -val 10 <test.bc >out_noval_001.bc && $llvmdis <out_noval_001.bc >out_noval_001.ll && $llc out_noval_001.bc -o out_noval_001.s && gcc -lrt out_noval_001.s -o out_noval_001.exe && chmod +x out_noval_001.exe
echo "END TEST"
echo " "
