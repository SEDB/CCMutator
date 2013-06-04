# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	$llvmdis: locatino of llvm-dis (for human readable output bitcode files)
#	$llc: location of llc (static LLVM compiler)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_PosixCondWait.so"
libraryName="PosixCondWait"

echo "BEGIN TEST: Find non verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName <test.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find verbose"
$opt -analyze -debug -load $llvmlibdir/$testLibName -$libraryName -verbose <test.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: Remove with no position values specified, out to out_m001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm <test.bc >out_m001.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 0, output to out01.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm -pos=0 <test.bc >out01.bc && $llc out01.bc -o out01.s && gcc -lrt out01.s -o out01.exe && chmod +x out01.exe
$llvmdis <out01.bc >out01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1 and 0, output to out11.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm -pos=1,0 <test.bc >out11.bc && $llc out11.bc -o out11.s && gcc -lrt out11.s -o out11.exe && chmod +x out11.exe
$llvmdis <out11.bc >out11.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove out of bounds position output to out_f001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -rm -pos=10000 <test.bc >out_f001.bc
$llvmdis <out_f001.bc >out_f001.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0 with no values specified, out to out_m001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -tmod -pos=0 <test.bc >out_m001.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0 with no nsec values specified, out to out_m001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -secval=1 -tmod -pos=0 <test.bc >out_m001.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify with no position values specified, out to out_m001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -secval=1 -tmod <test.bc >out_m001.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0: -1 second, +2000 nsec, out to out_m001.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -secval=-1 -nsecval=2000 -tmod -pos=0 <test.bc >out_m001.bc && $llc out_m001.bc -o out_m001.s && gcc -lpthread out_m001.s -o out_m001.exe && chmod +x out_m001.exe
$llvmdis <out_m001.bc >out_m001.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0: -1 second, +2000 nsec, ins point = 0, out to out_m002.bc (should fail)"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -inspt=0 -secval=-1 -nsecval=2000 -tmod -pos=0 <test.bc >out_m002.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0: -1 second, +2000 nsec, ins point = 24, out to out_m002.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -inspt=24 -secval=-1 -nsecval=2000 -tmod -pos=0 <test.bc >out_m002.bc && $llc out_m002.bc -o out_m002.s && gcc -lpthread out_m002.s -o out_m002.exe && chmod +x out_m002.exe
$llvmdis <out_m002.bc >out_m002.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode with no positions specified"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -switch <test.bc >/dev/null
$llvmdis <out_m002.bc >out_m002.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode with no positions specified"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -switch <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode and -rm"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -switch -rm -pos=0 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode with -secval positions specified"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -switch -pos=0 -secval=1 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode with -nsecval positions specified"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -switch -pos=0 -nsecval=1 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode position 0 out to out_002.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -switch -pos=0 <test.bc > out_002.bc && $llc out_002.bc -o out_002.s && gcc -lpthread out_002.s -o out_002.exe && chmod +x out_002.exe
$llvmdis <out_002.bc >out_002.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode position 0 and 1 out to out_003.bc"
$opt -debug -load $llvmlibdir/$testLibName -$libraryName -switch -pos=0,1 <test.bc > out_003.bc && $llc out_003.bc -o out_003.s && gcc -lpthread out_003.s -o out_003.exe && chmod +x out_003.exe
$llvmdis <out_003.bc >out_003.ll
echo "END TEST"
echo " "
