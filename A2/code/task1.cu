#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>

//Cuda kernel to do phase 1 and phase 2 of odd even sort
__global__ void oddEvenSortKernel(int *array, int size)
{
    int tid = threadIdx.x;         
    int totalPairs = size / 2;     // number of pairs in one phase

    for (int phase = 0; phase < size; phase++)
    {
        int phase_offset = phase % 2;

        // Cyclic distribution of pairs
        for (int pair = tid; pair < totalPairs; pair += blockDim.x)
        {
            int i = pair * 2 + phase_offset;

            if (i + 1 < size && array[i] > array[i + 1])
            {
                int temp = array[i];
                array[i] = array[i + 1];
                array[i + 1] = temp;
            }
        }

        __syncthreads();  // required for correctness
    }
}

void print_sort_status(std::vector<int> numbers)
{
    std::cout << "The input is sorted?: " << (std::is_sorted(numbers.begin(), numbers.end()) ? "True" : "False") << std::endl;
}



int main()
{
    constexpr unsigned int size = 100000; // Number of elements in the input

    // Initialize a vector with integers of value 0
    std::vector<int> numbers(size);
    // Populate our vector with (pseudo)random numbers
    srand(time(0));
    std::generate(numbers.begin(), numbers.end(), rand);

    int block_count = 1;
    int thread_count = 1024;   // hardware max

    print_sort_status(numbers);
    auto start = std::chrono::steady_clock::now();

    // Acloate the vector into the GPU
    int *d_numbers;
    cudaMalloc((void **)&d_numbers, size * sizeof(int));
    cudaMemcpy(d_numbers, numbers.data(), size * sizeof(int), cudaMemcpyHostToDevice);

    // Launch the kernel
    oddEvenSortKernel<<<block_count, thread_count>>>(d_numbers, size);

    cudaMemcpy(numbers.data(), d_numbers, size * sizeof(int), cudaMemcpyDeviceToHost);
    cudaFree(d_numbers);

    auto end = std::chrono::steady_clock::now();
    print_sort_status(numbers);
    std::cout << "Elapsed time =  " << std::chrono::duration<double>(end - start).count() << " sec\n";
}