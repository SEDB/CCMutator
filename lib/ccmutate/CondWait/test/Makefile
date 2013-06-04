LIBCXX_LOCATION=/home/markus/Repos/libc++/libc++

time: test_time.cpp
	$(clang) -g -S -std=c++0x -stdlib=libc++ -emit-llvm test_time.cpp -c -I$(LIBCXX_LOCATION)/include/c++/v1
	mv test_time.s test.ll
	llvm-as test.ll
	$(llc) test.bc -o test.s
	$(clang) -stdlib=libc++ test.s -o unmut.exe -I$(LIBCXX_LOCATION)/include/c++/v1 -L$(LIBCXX_LOCATION)/lib -lpthread -lsupc++ -lc++

cxx: test.cpp
	$(clang) -g -S -std=c++0x -stdlib=libc++ -emit-llvm test.cpp -c -I$(LIBCXX_LOCATION)/include/c++/v1
	mv test.s test.ll
	llvm-as test.ll
	$(llc) test.bc -o test.s
	$(clang) -stdlib=libc++ test.s -o unmut.exe -I$(LIBCXX_LOCATION)/include/c++/v1 -L$(LIBCXX_LOCATION)/lib -lpthread -lsupc++ -lc++

c: test_time.c
	$(clang) -g -S -emit-llvm test_time.c -c
	mv test_time.s test.ll
	llvm-as test.ll
	$(llc) test.bc -o test.s
	$(clang) test.s -o unmut.exe -lpthread

run:
	LD_PRELOAD=/home/markus/Repos/libc++/libc++/lib/libc++.so.1 ./unmut.exe $(A)

