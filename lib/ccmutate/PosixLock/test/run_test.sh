# Test script, requires that the following variables be present to the shell:
#	$opt: the location of opt
#	$llvmlibdir: the library directory of LLVM (where opt modules can be found)
#	$llvmdis: locatino of llvm-dis (for human readable output bitcode files)
#	$llc: location of llc (static LLVM compiler)
# Run make_test.sh prior to running this

# These run tests but the output of the tool needs to be checked by a human

testLibName="mutate_PosixLock.so"
libraryName="PosixLock"

echo "BEGIN TEST: Find non verbose"
$opt -basicaa -analyze -debug -load "$llvmlibdir"/"$testLibName" -$libraryName <test_local.bc >/dev/null
echo "END TEST: Find non verbose"
echo " "

echo "BEGIN TEST: Find verbose"
$opt -basicaa -analyze -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -verbose <test_local.bc >/dev/null
echo "END TEST: Find verbose"
echo " "

echo "BEGIN TEST: rmMode odd positions specified"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -rm -pos=0,1,7 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: rmMode out of bounds function"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -rm -pos=999,0 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: rmMode out of bounds pair"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -rm -pos=0,999 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: rmMode position (0,0) out to out_00.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -rm -pos=0,0 <test_local.bc >out_00.bc
$llvmdis <out_00.bc >out_00.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: rmMode positions (0,0) and (0,1) out to out_01.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -rm -pos=0,0 -pos=0,1 <test_local.bc >out_01.bc
$llvmdis <out_01.bc >out_01.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: rmMode and swapMode"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -rm -swap -pos=0,0 -pos=0,1 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: swap with 3 positions"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -swap -pos=0,0,99 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: swap with 5 positions"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -swap -pos=0,0,99,99,99 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: swap with first pair out-of-bounds"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -swap -pos=1000,1000,0,0 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: swap with second pair out-of-bounds"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -swap -pos=0,0,99,99 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: swap (0,0) with (0,2) out to out_02.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -swap -pos=0,0,0,2 <test_local.bc >out_02.bc
$llvmdis <out_02.bc >out_02.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: swap (0,0) with (0,1) out to out_03.bc (should have warning)"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -swap -pos=0,0,0,1 <test_local.bc >out_03.bc
$llvmdis <out_03.bc >out_03.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: shift with no positions"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: shift with no directions"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -pos=0,1 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: shift and remove"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -rm <test_local.bc >out_03.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: shift and swap"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -swap <test_local.bc >out_03.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: shift (0,0) lock up 200 positions out to out_04.bc (should fail to dominate uses)"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -lockdir=-200 <test_local.bc >out_04.bc
echo "END TEST"
echo " "

echo "BEGIN TEST: shift (0,0) lock up one position out to out_04.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -lockdir=-1 <test_local.bc >out_04.bc
$llvmdis <out_04.bc >out_04.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: shift (0,0) lock down two positions out to out_05.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -lockdir=2 <test_local.bc >out_05.bc
$llvmdis <out_05.bc >out_05.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: shift (0,0) lock down two positions and (0,2) up 1 out to out_06.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -pos=0,2 -lockdir=2,-1 <test_local.bc >out_06.bc
$llvmdis <out_06.bc >out_06.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: shift (0,0) lock down two positions and unlock down two out to out_07.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -lockdir=2 -unlockdir=2 <test_local.bc >out_07.bc
$llvmdis <out_07.bc >out_07.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: shift (0,0) lock down two positions and unlock down two out to and (0,2) lock up two and unlock down two out_08.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -pos=0,2 -lockdir=2,-2 -unlockdir=2,-2 <test_local.bc >out_08.bc
$llvmdis <out_08.bc >out_08.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: shift (0,0) lock and unlock down one position"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -pos=0,0 -lockdir=1 -unlockdir=1 <test_local.bc >out_08.bc
$llvmdis <out_08.bc >out_08.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: shift with -splitpos"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -shift -splitpos=3,7 -pos=0,0 -lockdir=1 -unlockdir=1 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: rm with -splitpos"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -rm -splitpos=3,7 -pos=0,0 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: swap with -splitpos"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -swap -splitpos=3,7 -pos=0,0,0,2 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split with no -splitpos"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0,0,2 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split with no positions"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -splitpos=3,7 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split with one position"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0 -splitpos=3,7 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (100,100) at (3,5) (too large, should warn)"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=100,100 -splitpos=3,5 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (0,0) at (50,3) (too large, should warn)"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0 -splitpos=50,3 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (0,0) at (3,50) (too large, should warn)"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0 -splitpos=3,50 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (0,0) at (3,5) and (0,2) and (3,50)"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0,0,2 -splitpos=3,5,3,4 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (0,0) at (3,5) and (0,2) and (3,4)"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0,0,2 -splitpos=3,5,4,3 <test_local.bc >/dev/null
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (0,0) at (3,5), out to out_09.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0 -splitpos=3,5 <test_local.bc >out_09.bc
$llvmdis <out_09.bc >out_09.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (0,0) at (5,3), out to out_10.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0 -splitpos=5,3 <test_local.bc >out_10.bc
$llvmdis <out_10.bc >out_10.ll
echo "END TEST"
echo " "

echo "BEGIN TEST: split position (0,0) at (5,3) and (0,2) at (2,3) out to out_11.bc"
$opt -basicaa -debug -load "$llvmlibdir"/"$testLibName" -$libraryName -split -pos=0,0,0,2 -splitpos=5,3,2,3 <test_local.bc >out_11.bc
$llvmdis <out_11.bc >out_11.ll
echo "END TEST"
echo " "
