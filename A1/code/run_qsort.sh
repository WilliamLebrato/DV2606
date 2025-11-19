#!/bin/bash

# Script to run qsort executables multiple times and calculate average execution time
# Usage: ./run_qsort.sh <executable1> [executable2] [executable3] ...

RUNS=4

if [ $# -eq 0 ]; then
    echo "Usage: $0 <executable1> [executable2] ..."
    echo "Example: $0 ./qsortseq ./qsortpar"
    exit 1
fi

# Function to run an executable multiple times and calculate average
run_benchmark() {
    local executable=$1
    local total_time=0

    echo "========================================="
    echo "Benchmarking: $executable"
    echo "========================================="

    if [ ! -f "$executable" ]; then
        echo "Error: $executable not found!"
        echo ""
        return
    fi

    if [ ! -x "$executable" ]; then
        echo "Error: $executable is not executable!"
        echo ""
        return
    fi

    echo "Running $RUNS times..."

    for i in $(seq 1 $RUNS); do
        echo -n "Run $i/$RUNS... "

        # Use time command and capture real time
        # Run the executable and extract the time
        elapsed=$( { time -p $executable > /dev/null 2>&1; } 2>&1 | grep real | awk '{print $2}' )

        echo "${elapsed}s"

        # Add to total (using bc for floating point arithmetic)
        total_time=$(echo "$total_time + $elapsed" | bc)
    done

    # Calculate average
    average=$(echo "scale=6; $total_time / $RUNS" | bc)

    echo "-----------------------------------------"
    echo "Total time: ${total_time}s"
    echo "Average time: ${average}s"
    echo ""
}

# Main execution
echo ""
echo "========================================="
echo "Quick Sort Benchmark"
echo "Number of runs per executable: $RUNS"
echo "========================================="
echo ""

# Run benchmark for each provided executable
for exe in "$@"; do
    run_benchmark "$exe"
done

echo "========================================="
echo "Benchmark complete!"
echo "========================================="
