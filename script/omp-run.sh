#!/bin/bash

iterations=1
input_name="circle"

if [[ $# == "1" ]] && [[ "$1" =~ ^[0-9]+$ ]]; then
    iterations="$1";
elif [[ $# == "1" ]] && [[ `find ./input/$1* | wc -l` == "1" ]]; then
    input_name="$1";
elif [[ $# == "2" ]] && [[ "$1" =~ ^[0-9]+$ ]] && [[ `find ./input/$2* | wc -l` == "1" ]]; then
    iterations="$1"
    input_name="$2";
elif [[ $# == "2" ]] && [[ "$2" =~ ^[0-9]+$ ]] && [[ `find ./input/$1* | wc -l` == "1" ]]; then
    iterations="$2"
    input_name="$1";
elif [[ $# != "0" ]]; then
    echo "You can pass the number of iterations and the input file"
    echo '"./omp-run.sh [N] [inputName]" or "./omp-run.sh [inputName] [N]"'
    exit 1;
fi

time_sum=0

echo "" > ./bench/omp/${input_name}.data

for (( i="0" ; $i<$iterations ; i=$i+1 )); do
    ./bin/omp-skyline < ./input/${input_name}*.in > ./output/omp/${input_name}.out 2>./.temp
    diff ./output/omp/${input_name}.out ./output/serial/${input_name}*.out;

    time_string=`tail -n 1 ./.temp`
    echo "Iteration #$(($i+1)), ${time_string}" | tee -a ./bench/omp/${input_name}.data
    time_sum=$(echo ${time_sum} + ${time_string:19} | bc)
done

echo "Average execution time (s): $(echo "scale=6; ${time_sum} / ${iterations}" | bc)" | tee -a ./bench/omp/${input_name}.data

rm -f ./.temp
