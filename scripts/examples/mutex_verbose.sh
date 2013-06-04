# Author: Markus Kusano
# preforms all possible combinations of mutations of removing mutex pairs on
# the passed filename ($1).
#
# The mutants are output to directory mutants/ in the same directory as $1

# Location of RSS CVS Repo root
CVS_PATH=/home/markus/Repos/src

# Location where LLVM is installed
LLVM_PATH="$CVS_PATH/install-3.2"

# Location where LLVM is built
LLVM_BUILD="$CVS_PATH/build-3.2"

# LLVM lib install directory
LLVM_LIB="$LLVM_PATH/lib"

# Location where ccmutate library files live
CCMUTATE_LIB="/home/markus/Repos/src/ccmutate/install/lib"

# opt binary location
OPT="$LLVM_PATH/bin/opt"


mut_mutex="$OPT -basicaa -debug -load $CCMUTATE_LIB/mutate_Mutex.so -Mutex"

$mut_mutex -verbose -analyze memcached-thread.o >/dev/null
