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

# combination binary location
COMBO="/home/markus/Repos/src/ccmutate/combinations/combinations"

# Location of inspect
#INSPECT_PATH="$CVS_PATH/inspect-0.3"

#INSPECT_LIB_PATH="$INSPECT_PATH/lib"

# Opt flags, taken from src/bin/inspect-cc
#OPT_FLAG="-load=$LLVM_BUILD/Release+Debug+Asserts/lib/LLVMTAP.so"
#OPT_FLAG="$OPT_FLAG -tap_inst_number"
#OPT_FLAG="$OPT_FLAG -basicaa"
#OPT_FLAG="$OPT_FLAG -tap_mem"   
#OPT_FLAG="$OPT_FLAG -tap_pthreads"

# LD flags, taken from stc/bin/inspect-cc
#LDFLAGS="$LDFLAGS -Wl,-rpath,$INSPECT_LIB_PATH"
#LDFLAGS="$LDFLAGS -Wl,-rpath-link,$INSPECT_LIB_PATH"
#LDFLAGS="$LDFLAGS -Wl,-L,$INSPECT_LIB_PATH"
#for l in  inspect pthread rt m stdc++ 
#do
#    LDFLAGS="$LDFLAGS -l$l"
#done

# When creating memcached (the executable) the following object files are linked:
#   memcached-memcached.o
#   memcached-slabs.o
#   memcached-items.o
#   memcached-assoc.o
#   memcached-thread.o
#   memcached-stats.o


echo "--------- Generating Swap Mutex Pair Mutants"
if [ "$1" == "" ]; then
    echo "Error: first command line option should be absolute/relative path to LLVM IR file to mutate"
    exit 1
fi

cd `dirname $1` || exit 1
mut_mutex="$OPT -basicaa -debug -load $CCMUTATE_LIB/mutate_Mutex.so -Mutex"
mkdir mutants
source=`basename $1` || exit 1
$mut_mutex -analyze <$source 1>/dev/null 2>out.txt || exit 1
sed -e '1d' out.txt >out_cut.txt # remove the line containing arguments
analyzeOut=(`cat out_cut.txt`)  # array with mutex pairs
rm out.txt || exit 1
analyzeOut[1]=$((analyzeOut[1] - 1))
echo ${analyzeOut[0]}
echo ${analyzeOut[1]}

# For each of the types of mutex pairs (analyzeOut[0]) preform each combination
# of first order mutex mutations
for (( i=0; i<=${analyzeOut[0]}; i++ ))
do
    # Pick groups of 2,4,..analyzeOut[1]
    for (( j=2; j<=${analyzeOut[1]}; j=j+2 ))
    do
        comboOut=( $($COMBO -p $i -c 0 -k "$j" ${analyzeOut[1]}) )
        arrSize=${#comboOut[@]}
        for (( k=0; k<$arrSize; k++ ))
        do
            $mut_mutex -swap -pos=${comboOut[k]} <$source >"mutants/${source}_swapMutex_${i}_${comboOut[k]}.o"
        done
        echo "Iteration: $j out of ${analyzeOut[1]}"
    done
done

# Link the final executable using clang
#echo "------ COMPILING INSTRUMENTED FILE WITH CLANG"
#echo "------ LDFLAGS = $LDFLAGS"
#clang  -g -emit-llvm $LDFLAGS -o memcached memcached-memcached.o \
#memcached-slabs.o memcached-items.o memcached-assoc.o \
#memcached-thread.o memcached-stats.o -levent 
#make install
#echo "------- INSPECT INSTRUMENTATION COMPLETE ------"
