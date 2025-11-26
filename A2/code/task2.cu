//task2.cu
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cuda.h>

__global__ void evenPhaseKernel(int *array, int size)
{
    int gid = blockIdx.x * blockDim.x + threadIdx.x;
    int totalPairs = size / 2;

    for (int pair = gid; pair < totalPairs; pair += gridDim.x * blockDim.x)
    {
        int i = pair * 2;

        if (i + 1 < size && array[i] > array[i + 1])
        {
            int temp = array[i];
            array[i] = array[i + 1];
            array[i + 1] = temp;
        }
    }
}


__global__ void oddPhaseKernel(int *array, int size)
{
    int gid = blockIdx.x * blockDim.x + threadIdx.x;
    int totalPairs = size / 2;

    for (int pair = gid; pair < totalPairs; pair += gridDim.x * blockDim.x)
    {
        int i = pair * 2 + 1;

        if (i + 1 < size && array[i] > array[i + 1])
        {
            int temp = array[i];
            array[i] = array[i + 1];
            array[i + 1] = temp;
        }
    }
}


void print_sort_status(const std::vector<int>& numbers)
{
    std::cout << "Sorted?: " << (std::is_sorted(numbers.begin(), numbers.end()) ? "Yes" : "No") << "\n";
}

int main()
{
    constexpr int size = 1 << 19;  // 524288 elements as in assignment
    std::vector<int> numbers(size);

    srand(time(0));
    std::generate(numbers.begin(), numbers.end(), rand);

    print_sort_status(numbers);
    auto start = std::chrono::steady_clock::now();

    // Allocate GPU memory
    int *d_numbers;
    cudaMalloc(&d_numbers, size * sizeof(int));
    cudaMemcpy(d_numbers, numbers.data(), size * sizeof(int), cudaMemcpyHostToDevice);

    // Launch configuration
    int threadsPerBlock = 1024;
    int numBlocks = 3;

    // TASK 2 LOOP: MULTIPLE KERNEL LAUNCHES
    for (int phase = 0; phase < size; phase++)
    {
        if (phase % 2 == 0)
            evenPhaseKernel<<<numBlocks, threadsPerBlock>>>(d_numbers, size);
        else
            oddPhaseKernel<<<numBlocks, threadsPerBlock>>>(d_numbers, size);
    }

    cudaMemcpy(numbers.data(), d_numbers, size * sizeof(int), cudaMemcpyDeviceToHost);
    cudaFree(d_numbers);

    auto end = std::chrono::steady_clock::now();
    print_sort_status(numbers);

    std::cout << "Elapsed time: "
              << std::chrono::duration<double>(end - start).count()
              << " sec\n";

    return 0;
}
