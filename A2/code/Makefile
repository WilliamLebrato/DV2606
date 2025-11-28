all: oddevensort task1 task2

oddevensort: oddevensort.cpp
	g++ oddevensort.cpp -o oddevensort -O3

task1: task1.cu
	nvcc task1.cu -o task1

task2: task2.cu
	nvcc task2.cu -o task2

sort: all
	./oddevensort
	./task1
	./task2