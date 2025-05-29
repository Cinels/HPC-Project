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
    echo '"./all-run.sh [N] [inputName]" or "./all-run.sh [inputName] [N]"'
    exit 1;
fi

for (( num_threads="1" ; $num_threads<"5" ; num_threads=$num_threads+1 )); do

	serial_time_sum=0
	omp_time_sum=0
	mpi_time_sum=0

	echo "" > ./bench/omp/${input_name}/p${num_threads}.data
	echo "" > ./bench/mpi/${input_name}/p${num_threads}.data

	echo "p=$num_threads               omp       mpi"
	for (( i="0" ; $i<$iterations ; i=$i+1 )); do
	    OMP_NUM_THREADS=${num_threads} ./bin/omp-skyline < ./input/${input_name}*.in > ./output/serial-omp/${input_name}.out 2>./.omptemp
	    mpirun --stdin 0 -n ${num_threads} ./bin/mpi-skyline < ./input/${input_name}*.in > ./output/serial-mpi/${input_name}.out 2>./.mpitemp

	    diff ./output/omp/${input_name}.out ./output/serial/${input_name}*.out
	    diff ./output/mpi/${input_name}.out ./output/serial/${input_name}*.out

	#    serial_time_string=`tail -n 1 ./.serialtemp`
	    omp_time_string=`tail -n 1 ./.omptemp`
	    mpi_time_string=`tail -n 1 ./.mpitemp`

	    iteration_string="Iteration #$(($i+1)), "
	    echo "${iteration_string}${omp_time_string}" >> ./bench/omp/${input_name}/p${num_threads}.data
	    echo "${iteration_string}${mpi_time_string}" >> ./bench/mpi/${input_name}/p${num_threads}.data

	    echo "${iteration_string} ${omp_time_string:19}, ${mpi_time_string:19}"
	    omp_time_sum=$(echo ${omp_time_sum} + ${omp_time_string:19} | bc)
	    mpi_time_sum=$(echo ${mpi_time_sum} + ${mpi_time_string:19} | bc)

	done

	echo "Average execution time (s): $(echo "scale=6; x=${omp_time_sum} / ${iterations}; if(x<1) print 0; x" | bc)" >> ./bench/omp/${input_name}/p${num_threads}.data
	echo "Average execution time (s): $(echo "scale=6; x=${mpi_time_sum} / ${iterations}; if(x<1) print 0; x" | bc)" >> ./bench/mpi/${input_name}/p${num_threads}.data

	echo "p=$num_threads              omp       mpi"
	echo "Avg time (s):  $(echo "scale=6; x=${omp_time_sum} / ${iterations}; if(x<1) print 0; x" | bc), $(echo "scale=6; x=${mpi_time_sum} / ${iterations}; if(x<1) print 0; x" | bc)"
	echo ""

done


rm -f ./.*temp
