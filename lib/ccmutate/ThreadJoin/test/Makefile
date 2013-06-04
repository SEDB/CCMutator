# Makes the test bitcode files
# Requires that the following variables be present to the shell
#   $clang: the location of clang
#   $llvmdis: the location of llvm-dis (required for human readable test bitcode files)

LIBCXX_LOCATION=/home/markus/Repos/libc++/libc++

cxx: test_c++11.cpp
	$(clang) -g -S -std=c++0x -stdlib=libc++ -emit-llvm test_c++11.cpp -c -I$(LIBCXX_LOCATION)/include/c++/v1
	mv test_c++11.s test_c++11.ll
	llvm-as test_c++11.ll
	mv test_c++11.bc test.bc

c: test.c
	$(clang) -g -emit-llvm test.c -c -o test.bc

test:
	$(clang) -stdlib=libc++ out_s011_0.s -I$(LIBCXX_LOCATION)/include/c++/v1 -L$(LIBCXX_LOCATION)/lib -lpthread -lsupc++ -lc++
