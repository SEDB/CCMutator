CCMUTATE_LIB="/home/markus/Repos/src/ccmutate/install/lib"

testLibName="mutate_Fence.so"
libraryName="-Fence"

bitcode="test.bc"

echo "Begin Test: Non verbose analyze"
opt -analyze -load $CCMUTATE_LIB/$testLibName $libraryName <test.bc >/dev/null
echo ""

echo "Begin Test: verbose analyze"
opt -analyze -load $CCMUTATE_LIB/$testLibName $libraryName -verbose <test.bc >/dev/null
echo ""

echo "Begin Test: remove with no positions speciied (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -rm <test.bc >/dev/null
echo ""

echo "Begin Test: remove out out bounds position (should warn)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -rm -pos=999 <test.bc >/dev/null
echo ""

echo "Begin Test: remove position 0, out to out_001.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -rm -pos=0 <test.bc >out_001.bc
llvm-dis out_001.bc 
echo ""

echo "Begin Test: mod and rm (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -rm -mod -pos=0 <test.bc >/dev/null
echo ""

echo "Begin Test: mod with no -pos (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -order=0 <test.bc >/dev/null
echo ""

echo "Begin Test: mod with no -order (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -pos=0  <test.bc >/dev/null
echo ""

echo "Begin Test: mod position out of bounds (should warn)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -order=0 -pos=999  <test.bc >/dev/null
echo ""

echo "Begin Test: mod order out of bounds (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -order=4 -pos=999  <test.bc >/dev/null
echo ""

echo "Begin Test: mod position 0 to release, out to out_002.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -order=1 -pos=0  <test.bc >out_002.bc
llvm-dis out_002.bc
echo ""

echo "Begin Test: mod position 0,1,2 to seq_cst, out to out_003.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -order=3 -pos=0,1,2  <test.bc >out_003.bc
llvm-dis out_003.bc
echo ""

echo "Begin Test: -scope and -rm (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope -rm -pos=0 <test.bc >/dev/null
echo ""

echo "Begin Test: -scope and -mod (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope -mod -pos=0 <test.bc >/dev/null
echo ""

echo "Begin Test: -scope and -mod and -rm (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -rm -scope -mod -pos=0 <test.bc >/dev/null
echo ""

echo "Begin Test: -scope with no -pos (should fail"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope <test.bc >/dev/null
echo ""

echo "Begin Test: -scope position 1 out to out_004.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope -pos=1 <test.bc >out_004.bc
llvm-dis out_004.bc
echo ""

echo "Begin Test: -scope position 1 (of out_004.bc) out to out_005.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope -pos=1 <out_004.bc >out_005.bc
llvm-dis out_005.bc
echo ""

echo "Begin Test: mod position 0 to acquire release, out to out_006.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -order=2 -pos=0  <test.bc >out_006.bc
llvm-dis out_006.bc
echo ""
