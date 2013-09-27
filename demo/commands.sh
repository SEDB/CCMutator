LLVMSO="/home/markus/src/CCMutator/install/lib/mutate_Mutex.so"

# Enumerate non-verbose
#/home/markus/src/install-3.2/bin/opt -basicaa -analyze -load $LLVMSO -Mutex <memcached-thread.bc >/dev/null

# Enumerate verbose
#/home/markus/src/install-3.2/bin/opt -basicaa -analyze -load $LLVMSO -Mutex -verbose <memcached-thread.bc >/dev/null

# Remove lock unlock pair 0,1
#/home/markus/src/install-3.2/bin/opt -basicaa -load $LLVMSO -Mutex -rm -pos=0,1 <memcached-thread.bc >memcached-thread_rm-01.bc

# Enumerate verbosely the resulting mutant
#/home/markus/src/install-3.2/bin/opt -basicaa -analyze -load $LLVMSO -Mutex -verbose <memcached-thread_rm-01.bc >/dev/null

# Split critical section 0,8 of previous mutant
#/home/markus/src/install-3.2/bin/opt -basicaa -load $LLVMSO -Mutex  -pos=0,8 -split -splitpos=3,6 <memcached-thread_rm-01.bc >memcached-thread_rm-01_split-09.bc
#llvm-dis memcached-thread_rm-01_split-09.bc

# Enumerate verbosely the resulting mutant
#/home/markus/src/install-3.2/bin/opt -basicaa -analyze -load $LLVMSO -Mutex -verbose <memcached-thread_rm-01_split-09.bc >/dev/null

# Swap a bunch of locks in the non-mutated version
#/home/markus/src/install-3.2/bin/opt -basicaa -load $LLVMSO -Mutex -swap -pos=0,1,0,2,0,5,0,8 <memcached-thread.bc >memcached-thread_rm-01.bc

# Generate all the rm mutants
../scripts/mutate_rmMutex.sh memcached-thread.bc
