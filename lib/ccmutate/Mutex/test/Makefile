# Makes the test bitcode files
# Requires that the following variables be present to the shell
#   $clang: the location of clang
#   $llvmdis: the location of llvm-dis (required for human readable test bitcode files)

LIBCXX_LOCATION=/home/markus/Repos/libc++/libc++

cxx: test.cpp
	$(clang) -g -S -std=c++0x -stdlib=libc++ -emit-llvm test.cpp -c -I$(LIBCXX_LOCATION)/include/c++/v1
	mv test.s test.ll
	llvm-as test.ll
	$(llc) test.bc -o test.s
	$(clang) -stdlib=libc++ test.s -o unmut.exe -I$(LIBCXX_LOCATION)/include/c++/v1 -L$(LIBCXX_LOCATION)/lib -lpthread -lsupc++ -lc++

run:
	LD_PRELOAD=/home/markus/Repos/libc++/libc++/lib/libc++.so.1 ./unmut.exe $(A)
	LD_PRELOAD=/home/markus/Repos/libc++/libc++/lib/libc++.so.1 ./test_out_03.exe $(A)
	LD_PRELOAD=/home/markus/Repos/libc++/libc++/lib/libc++.so.1 ./test_out_08.exe $(A)
	LD_PRELOAD=/home/markus/Repos/libc++/libc++/lib/libc++.so.1 ./test_out_11.exe $(A)

test:
	$(llc) out_03.bc -o test_out_03.s
	$(clang) -stdlib=libc++ test_out_03.s -o test_out_03.exe -I$(LIBCXX_LOCATION)/include/c++/v1 -L$(LIBCXX_LOCATION)/lib -lpthread -lsupc++ -lc++
	$(llc) out_08.bc -o test_out_08.s
	$(clang) -stdlib=libc++ test_out_08.s -o test_out_08.exe -I$(LIBCXX_LOCATION)/include/c++/v1 -L$(LIBCXX_LOCATION)/lib -lpthread -lsupc++ -lc++
	$(llc) out_11.bc -o test_out_11.s
	$(clang) -stdlib=libc++ test_out_11.s -o test_out_11.exe -I$(LIBCXX_LOCATION)/include/c++/v1 -L$(LIBCXX_LOCATION)/lib -lpthread -lsupc++ -lc++
