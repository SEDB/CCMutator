LIBCXX_LOCATION=/home/markus/Repos/libc++/libc++

all:
	clang -g -S -std=c++0x -stdlib=libc++ -emit-llvm test.cpp -c -I$(LIBCXX_LOCATION)/include/c++/v1
	mv test.s test.ll
	llvm-as test.ll
