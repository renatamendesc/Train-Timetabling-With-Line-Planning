#include <iostream>
#include <vector>
#include <chrono>
#include <omp.h>

int main() {
    const int N = 1 << 20; // 1M elementos
    std::vector<float> a(N), b(N), c(N);

    // Inicializa vetores
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(2 * i);
    }

    // Marca início do tempo de soma
    auto start = std::chrono::high_resolution_clock::now();

    // Soma paralela
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        c[i] = a[i] + b[i];
    }

    // Marca fim
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> duration = end - start;

    std::cout << "Tempo de execu\xC3\xA7\xC3\xA3o na CPU: " << duration.count() << " ms\n";

    // Resultados (primeiros 5)
    std::cout << "Resultados (primeiros 5 elementos):\n";
    for (int i = 0; i < 5; ++i) {
        std::cout << a[i] << " + " << b[i] << " = " << c[i] << "\n";
    }

    return 0;
}
