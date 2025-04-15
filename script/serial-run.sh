#!/bin/bash

ITERATIONS=1
TEST="circle"

if [[ $# == "1" ]] && [[ "$1" =~ ^[0-9]+$ ]]; then
    ITERATIONS="$1";
elif [[ $# == "1" ]]; then
    TEST="$1";
elif [[ $# == "2" ]] && [[ "$1" =~ ^[0-9]+$ ]]; then
    ITERATIONS="$1"
    TEST="$2";
elif [[ $# == "2" ]] && [[ "$2" =~ ^[0-9]+$ ]]; then
    ITERATIONS="$2"
    TEST="$1";
elif [[ $# != "0" ]]; then
    echo "You can pass the number of iterations and the input file"
    echo '"./serial-run.sh [N] [inputName]"';
fi

time_sum=0

echo "" > ./bench/${TEST}-serial.data

for (( i="0" ; $i<$ITERATIONS ; i=$i+1 )); do
    ./bin/omp-skyline < ./input/${TEST}*.in > ./output/${TEST}-serial.out 2>./temp
    diff ./output/${TEST}-serial.out ./output/serial/${TEST}*.out;

    time_string=`tail -n 1 ./temp`
    echo "Iteration #$(($i+1)), ${time_string}" | tee -a ./bench/${TEST}-serial.data
    time_sum=$(echo ${time_sum} + ${time_string:19} | bc)
done

echo "Average execution time (s): $(echo "scale=6; ${time_sum} / ${ITERATIONS}" | bc)" | tee -a ./bench/${TEST}-serial.data

rm -f ./temp
