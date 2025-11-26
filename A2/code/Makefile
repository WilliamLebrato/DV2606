all: oddevensort task1 task2

oddevensort: oddevensort.cpp
	g++ oddevensort.cpp -o oddevensort

task1: task1.cu
	nvcc task1.cu -o task1

task2: task2.cu
	nvcc task2.cu -o task2

run: all
	./oddevensort
	./task1
	./task2