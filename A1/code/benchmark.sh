#!/bin/bash

GREEN="\033[0;32m"
RED="\033[0;31m"
END="\033[0m"

echo "Testing accuracy test with Gaussian elimination"
echo "---------------------------------"

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <sequential_executable> <parallel_executable>"
    exit 1
fi

seq_executable=$1
par_executable=$2

for n in 2 4 8 16 32 64 128 256 512 1024 2048; do
    echo "Testing with n=$n"

    seq_result=$(./"$seq_executable" -n $n -P 1 -I fast)
    par_result=$(./"$par_executable" -n $n -P 1 -I fast)

    seq_matrix_a=$(echo "$seq_result" | grep -A $n "Matrix A:" | grep -v "Matrix A:")
    par_matrix_a=$(echo "$par_result" | grep -A $n "Matrix A:" | grep -v "Matrix A:")

    seq_vector_b=$(echo "$seq_result" | grep -A 1 "Vector b:" | grep -v "Vector b:")
    par_vector_b=$(echo "$par_result" | grep -A 1 "Vector b:" | grep -v "Vector b:")

    seq_vector_y=$(echo "$seq_result" | grep -A 1 "Vector y:" | grep -v "Vector y:")
    par_vector_y=$(echo "$par_result" | grep -A 1 "Vector y:" | grep -v "Vector y:")

    matrix_a_result=$(diff <(echo "$seq_matrix_a") <(echo "$par_matrix_a") > /dev/null && echo "Passed" || echo "Failed")
    vector_b_result=$(diff <(echo "$seq_vector_b") <(echo "$par_vector_b") > /dev/null && echo "Passed" || echo "Failed")
    vector_y_result=$(diff <(echo "$seq_vector_y") <(echo "$par_vector_y") > /dev/null && echo "Passed" || echo "Failed")

    echo "Comparison results for n=$n:"
    echo -e "- Matrix A: ${GREEN}$matrix_a_result${END}"
    echo -e "- Vector b: ${GREEN}$vector_b_result${END}"
    echo -e "- Vector y: ${GREEN}$vector_y_result${END}"
done
