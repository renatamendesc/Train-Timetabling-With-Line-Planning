// quickstart_cuda_optimized.cu
// Exemplo otimizado de paralelização com CUDA na GPU
// Compile: nvcc -std=c++14 quickstart_cuda_optimized.cu -o quickstart_cuda
// Execute: ./quickstart_cuda

#include <iostream>
#include <cuda_runtime.h>

// Kernel simples: soma vetor a + b -> c
__global__ void vectorAdd(const float* a, const float* b, float* c, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}

int main() {
    const int N = 1 << 20; // 1M elementos
    const size_t size = N * sizeof(float);

    // Alocação de memória pinned no host
    float *h_a = nullptr, *h_b = nullptr, *h_c = nullptr;
    cudaMallocHost(&h_a, size);
    cudaMallocHost(&h_b, size);
    cudaMallocHost(&h_c, size);

    // Inicializa vetores no host
    for (int i = 0; i < N; ++i) {
        h_a[i] = static_cast<float>(i);
        h_b[i] = static_cast<float>(2 * i);
    }

    // Alocação de memória na GPU (device)
    float *d_a = nullptr, *d_b = nullptr, *d_c = nullptr;
    cudaMalloc(&d_a, size);
    cudaMalloc(&d_b, size);
    cudaMalloc(&d_c, size);

    // Cria stream CUDA
    cudaStream_t stream;
    cudaStreamCreate(&stream);

    // Cópia assíncrona dos dados do host para o device
    cudaMemcpyAsync(d_a, h_a, size, cudaMemcpyHostToDevice, stream);
    cudaMemcpyAsync(d_b, h_b, size, cudaMemcpyHostToDevice, stream);

    // Configuração da execução do kernel
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;

    // Marca eventos para medir tempo do kernel
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start, stream);

    // Execução do kernel no stream
    vectorAdd<<<blocksPerGrid, threadsPerBlock, 0, stream>>>(d_a, d_b, d_c, N);

    cudaEventRecord(stop, stream);

    // Cópia assíncrona do resultado para o host
    cudaMemcpyAsync(h_c, d_c, size, cudaMemcpyDeviceToHost, stream);

    // Espera todas as operações terminarem
    cudaStreamSynchronize(stream);

    // Calcula tempo do kernel
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    std::cout << "Tempo de execu\xC3\xA7\xC3\xA3o do kernel: " << milliseconds << " ms\n";

    // Verifica resultado (Exemplo: imprime primeiros 5 elementos)
    std::cout << "Resultados (primeiros 5 elementos):\n";
    for (int i = 0; i < 5; ++i) {
        std::cout << h_a[i] << " + " << h_b[i] << " = " << h_c[i] << std::endl;
    }

    // Libera recursos
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);
    cudaFreeHost(h_a);
    cudaFreeHost(h_b);
    cudaFreeHost(h_c);
    cudaStreamDestroy(stream);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return 0;
}
