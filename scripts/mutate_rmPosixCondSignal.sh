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


echo "--------- Generating rm cond_{signal,broadcast} mutants"
if [ "$1" == "" ]; then
    echo "Error: first command line option should be absolute/relative path to LLVM IR file to mutate"
    exit 1
fi

cd `dirname $1`
mutate="$OPT -debug -load $CCMUTATE_LIB/mutate_PosixCondSignal.so -PosixCondSignal"
mkdir mutants
source=`basename $1`

echo "source == $source"

$mutate -analyze <$source 1>/dev/null 2>out.txt
analyzeOut=`cat out.txt | tail -n 1`
analyzeOut=$((analyzeOut -1))
echo "analyzeOut == $analyzeOut"
rm out.txt

# For each of the possible combinations preform first order remove cond_broadcast or cond_signal
# Pick groups of 1,2,..analyzeOut
for (( i=1; i<=$analyzeOut; i++ ))
do
    echo "pwd == `pwd`"

    echo "i == $i"
    comboOut=( $($COMBO      -c 0 -k "$i" $analyzeOut ) )
    echo "comboOut == ${comboOut[@]}"
    arrSize=${#comboOut[@]}
    for (( k=0; k<$arrSize; k++ ))
    do
        $mutate -rmmode -pos=${comboOut[k]} <$source >mutants/${source}_rmCondSignal_${comboOut[k]}.o
    done
    echo "Iteration: $j out of $analyzeOut"
done

# Link the final executable using clang
#echo "------ COMPILING INSTRUMENTED FILE WITH CLANG"
#echo "------ LDFLAGS = $LDFLAGS"
#clang  -g -emit-llvm $LDFLAGS -o memcached memcached-memcached.o \
#memcached-slabs.o memcached-items.o memcached-assoc.o \
#memcached-thread.o memcached-stats.o -levent 
#make install
#echo "------- INSPECT INSTRUMENTATION COMPLETE ------"
