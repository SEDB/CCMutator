CCMUTATE_LIB="/home/markus/Repos/src/ccmutate/install/lib"

testLibName="mutate_Store.so"
libraryName="-Store"

echo "Begin Test: Non verbose analyze"
opt -analyze -load $CCMUTATE_LIB/$testLibName $libraryName <test.bc >/dev/null
echo ""

echo "Begin Test: verbose analyze"
opt -analyze -load $CCMUTATE_LIB/$testLibName $libraryName -verbose <test.bc >/dev/null
echo ""

echo "Begin Test: -toggle with no -pos (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -toggle <test.bc >/dev/null
echo ""

echo "Begin Test: -toggle position 0 out to out_001.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -toggle -pos=0 <test.bc >out_001.bc
llvm-dis out_001.bc
echo ""

echo "Begin Test: -toggle 1 out of bounds (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -toggle -pos=3 <test.bc >/dev/null
echo ""

echo "Begin Test: -mod without -pos (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -order=0 <test.bc >/dev/null
echo ""

echo "Begin Test: -mod and -toggle (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -toggle -order=0 -pos=0 <test.bc >/dev/null
echo ""

echo "Begin Test: -mod and no -order (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -toggle -pos=0 <test.bc >/dev/null
echo ""

echo "Begin Test: just -mod (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod <test.bc >/dev/null
echo ""

echo "Begin Test: -mod position 0 to unordered (-order=0) out to out_002.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -pos=0 -order=0 <test.bc >out_002.bc
llvm-dis out_002.bc
echo ""


echo "Begin Test: -mod position 1 to monotonic (-order=1) out to out_003.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -pos=1 -order=1 <test.bc >out_003.bc
llvm-dis out_003.bc
echo ""


echo "Begin Test: -mod position 0 to acquire (-order=2) out to out_004.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -pos=0 -order=2 <test.bc >out_004.bc
llvm-dis out_004.bc
echo ""

echo "Begin Test: -mod position 0 to seq cst (-order=3) out to out_005.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -pos=0 -order=3 <test.bc >out_005.bc
llvm-dis out_005.bc
echo ""

echo "Begin Test: -mod position 0 to order=4 (off by 1, should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -pos=0 -order=4 <test.bc >/dev/null
echo ""

echo "Begin Test: -mod position 0,1,2 to order=3,0 out to out_006.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -pos=0,1,2 -order=3,0 <test.bc >out_006.bc
llvm-dis out_006.bc
echo ""

echo "Begin Test: -mod and -scope (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -mod -scope -pos=0 -order=0<test.bc >/dev/null
echo ""

echo "Begin Test: -scope and -toggle (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope -toggle -pos=0<test.bc >/dev/null
echo ""

echo "Begin Test: -scope and no -pos (should fail)"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope <test.bc >/dev/null
echo ""

echo "Begin Test: -scope position 0, out to out_007.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope -pos=0 <test.bc >out_007.bc
llvm-dis out_007.bc
echo ""

echo "Begin Test: -scope position 0 of out_007.bc out to out_008.bc"
opt -load $CCMUTATE_LIB/$testLibName $libraryName -scope -pos=0 <out_007.bc >out_008.bc
llvm-dis out_008.bc
echo ""
