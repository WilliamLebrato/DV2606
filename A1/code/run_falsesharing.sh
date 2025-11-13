#!/bin/bash

echo "Compiling and running falsesharing variants with no optimization..."

# Compile all programs
gcc -O0 falsesharing.c -o falsesharing -lpthread
gcc -O0 falsesharing_a.c -o falsesharing_a -lpthread
gcc -O0 falsesharing_b.c -o falsesharing_b -lpthread
gcc -O0 falsesharing_c.c -o falsesharing_c -lpthread

echo "Running falsesharing (original):"
for i in {1..5}
do
    echo "Run $i:"
    time ./falsesharing
    echo ""
done

echo "Running falsesharing_a (a local):"
for i in {1..5}
do
    echo "Run $i:"
    time ./falsesharing_a
    echo ""
done

echo "Running falsesharing_b (b local):"
for i in {1..5}
do
    echo "Run $i:"
    time ./falsesharing_b
    echo ""
done

echo "Running falsesharing_c (c local):"
for i in {1..5}
do
    echo "Run $i:"
    time ./falsesharing_c
    echo ""
done