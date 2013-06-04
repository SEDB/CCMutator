# combination binary location
COMBO="/home/markus/Repos/src/ccmutate/combinations/combinations"

CCMUTATE_LIB="/home/markus/Repos/src/ccmutate/install/lib"

echo "--------- Generating Remove Mutex Pair Mutants"
if [ "$1" == "" ]; then
    echo "Error: first command line option should be absolute/relative path to LLVM IR file to mutate"
    exit 1
fi

cd `dirname $1` || exit 1
mut_mutex="opt -basicaa -load $CCMUTATE_LIB/mutate_Mutex.so -Mutex"
mkdir mutants
source=`basename $1` || exit 1
$mut_mutex -analyze <$source 1>/dev/null 2>out.txt || exit 1
analyzeOut=(`cat out.txt`)  # array with mutex pairs
rm out.txt || exit 1
analyzeOut[1]=$((analyzeOut[1] - 1))
echo ${analyzeOut[0]}
echo ${analyzeOut[1]}

# For each of the types of mutex pairs (analyzeOut[0]) preform each combination
# of first order mutex mutations
for (( i=0; i<=${analyzeOut[0]}; i++ ))
do
    # Pick groups of 1,2,...analyzeOut[1]
    for (( j=1; j<=${analyzeOut[1]}; j++ ))
    do
        comboOut=( $($COMBO -p $i -c 0 -k "$j" ${analyzeOut[1]}) )
        arrSize=${#comboOut[@]}
        for (( k=0; k<$arrSize; k++ ))
        do
            $mut_mutex -rm -pos=${comboOut[k]} <$source >"mutants/${source}_rmMutex_${i}_${comboOut[k]}.o"
        done
        echo "Iteration: $j out of ${analyzeOut[1]}"
    done
done
