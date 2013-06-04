# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	$llvmdis: locatino of llvm-dis (for human readable output bitcode files)
#	$llc: location of llc (static LLVM compiler)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_CondWait.so"
libraryName="CondWait"

llvmlibdir="/home/markus/Repos/src/ccmutate/install/lib"
opt="opt"
llvmdis="llvm-dis"
llc="llc"


echo "BEGIN TEST: Find non verbose posix and cpp"
$opt -analyze  -load $llvmlibdir/$testLibName -$libraryName -posix -cpp <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Find verbose posix and cpp"
$opt -analyze  -load $llvmlibdir/$testLibName -$libraryName -verbose -posix -cpp <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Find verbose cpp"
$opt -analyze  -load $llvmlibdir/$testLibName -$libraryName -verbose -cpp <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Find verbose posix"
$opt -analyze  -load $llvmlibdir/$testLibName -$libraryName -verbose -posix <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Find verbose neither cpp nor posix (should fail)"
$opt -analyze  -load $llvmlibdir/$testLibName -$libraryName -verbose <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove with no position values specified (should fail)"
$opt  -load $llvmlibdir/$testLibName -$libraryName -rm -cpp -posix <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 0, output to out01.bc"
$opt  -load $llvmlibdir/$testLibName -$libraryName -rm -cpp -posix -pos=0 <test.bc >out01.bc
$llvmdis <out01.bc >out01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove position 1 and 0, output to out11.bc"
$opt  -load $llvmlibdir/$testLibName -$libraryName -rm -posix -cpp -pos=1,0 <test.bc >out11.bc
$llvmdis <out11.bc >out11.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Remove out of bounds position (should fail)"
$opt  -load $llvmlibdir/$testLibName -$libraryName -rm -cpp -pos=3 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Find non verbose posix and cpp"
$opt -analyze  -load $llvmlibdir/$testLibName -$libraryName -posix -cpp <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0 with no values specified (should fail)"
$opt  -load $llvmlibdir/$testLibName -$libraryName -posix -tmod -pos=0 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0 with no nsec values specified, (should fail)"
$opt  -load $llvmlibdir/$testLibName -$libraryName -secval=1 -posix -tmod -pos=0 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify with no position values specified (should fail)"
$opt  -load $llvmlibdir/$testLibName -$libraryName -secval=1 -posix -tmod <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0: -1 second, +2000 nsec, out to out_m001.bc"
$opt  -load $llvmlibdir/$testLibName -$libraryName -secval=-1 -posix -nsecval=2000 -tmod -pos=1 <test.bc >out_m001.bc && llc out_m001.bc -o out_m001.s && gcc -lpthread out_m001.s -o out_m001.exe && chmod +x out_m001.exe
$llvmdis <out_m001.bc >out_m001.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0: -1 second, +2000 nsec, ins point = 0, out to out_m002.bc (should fail)"
$opt  -load $llvmlibdir/$testLibName -$libraryName -inspt=0 -secval=-1 -nsecval=2000 -posix -tmod -pos=0 <test.bc >out_m0#2.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: Modify position 0: -1 second, +2000 nsec, ins point = 24, out to out_m002.bc"
$opt  -load $llvmlibdir/$testLibName -$libraryName -inspt=24 -secval=-1 -nsecval=2000 -tmod -posix -pos=0 <test.bc >out_m002.bc && $llc out_m002.bc -o out_m002.s && gcc -lpthread out_m002.s -o out_m002.exe && chmod +x out_m002.exe
$llvmdis <out_m002.bc >out_m002.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode with no positions specified (should fail)"
$opt  -load $llvmlibdir/$testLibName -$libraryName -switch -posix <test.bc >/dev/null
$llvmdis <out_m002.bc >out_m002.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode with no positions specified"
$opt  -load $llvmlibdir/$testLibName -$libraryName -switch <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode and -rm"
$opt  -load $llvmlibdir/$testLibName -$libraryName -switch -posix -rm -pos=0 <test.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: switch mode with -secval positions specified"
$opt  -load $llvmlibdir/$testLibName -$libraryName -switch -pos=0 -posix -secval=1 <test.bc >/dev/null
echo "END TEST"
echo " "

#echo "BEGIN TEST: switch mode with -nsecval positions specified"
#$opt  -load $llvmlibdir/$testLibName -$libraryName -switch -pos=0 -nsecval=1 <test.bc >/dev/null
#echo "END TEST"
#echo " "
#
#echo "BEGIN TEST: switch mode position 0 out to out_002.bc"
#$opt  -load $llvmlibdir/$testLibName -$libraryName -switch -pos=0 <test.bc > out_002.bc && $llc out_002.bc -o out_#002.s && gcc -lpthread out_002.s -o out_002.exe && chmod +x out_002.exe
#$llvmdis <out_002.bc >out_002.ll
#echo "END TEST"
#echo " "
#
echo "BEGIN TEST: switch mode position 0 and 1 out to out_003.bc"
$opt  -load $llvmlibdir/$testLibName -$libraryName -cpp -switch -pos=0,1 <test.bc > out_003.bc && $llc out_003.bc -o ou_003.s && gcc -lpthread out_003.s -o out_003.exe && chmod +x out_003.exe
$llvmdis <out_003.bc >out_003.ll
echo "END TEST"
echo " "
