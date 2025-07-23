#include <iostream>
#include <thread>
#include <cuda_runtime.h>

int main() {
    // Obter o número de threads da CPU
    unsigned int cpu_threads = std::thread::hardware_concurrency();
    std::cout << "Número de threads da CPU: " << cpu_threads << std::endl;

    // Obter o número de threads da GPU (CUDA)
    int device_count;
    cudaGetDeviceCount(&device_count);
    if (device_count > 0) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0); // Supondo o primeiro dispositivo GPU
        int max_threads_per_block = prop.maxThreadsPerBlock;
        int max_blocks_per_multiprocessor = prop.maxBlocksPerMultiProcessor;
        int multiprocessor_count = prop.multiProcessorCount;
        // Estimativa do número total de threads na GPU
        int gpu_threads = max_threads_per_block * max_blocks_per_multiprocessor * multiprocessor_count;
        std::cout << "Número estimado de threads da GPU: " << gpu_threads << std::endl;
    } else {
        std::cout << "Nenhuma GPU CUDA encontrada." << std::endl;
    }

    return 0;
}