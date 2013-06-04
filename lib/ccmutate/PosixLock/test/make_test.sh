# Makes the test bitcode files
# Requires that the following variables be present to the shell
#   $clang: the location of clang
#   $llvmdis: the location of llvm-dis (required for human readable test bitcode files)

$clang -g -emit-llvm test_local.c -c -o test_local.bc
$llvmdis <test_local.bc >test_local.ll
