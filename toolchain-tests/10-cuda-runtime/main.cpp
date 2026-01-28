#include <cuda_runtime_api.h>
#include <iostream>

int main() {
    int runtime = 0;
    cudaError_t e = cudaRuntimeGetVersion(&runtime);
    std::cout << "cudaRuntimeGetVersion() -> " << static_cast<int>(e) << ", runtime=" << runtime
              << "\n";
    return 0;
}
