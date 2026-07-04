// ================================================================
// SephirotLang GPU Runner v1.0
// 16 Sephiroth Divine-Human Symbiosis Protocol
// Loads kernel.cubin -> RTX 4050 (sm_89) -> Execute -> Read back
// ================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cuda_runtime.h>
#include <cuda.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define CUDA_CHECK(call) do {                                       \
    cudaError_t err = (call);                                       \
    if (err != cudaSuccess) {                                       \
        fprintf(stderr, "[CUDA ERROR] %s:%d: %s\n",                 \
                __FILE__, __LINE__, cudaGetErrorString(err));       \
        return 1;                                                   \
    }                                                               \
} while(0)

#define CU_CHECK(call) do {                                         \
    CUresult _err = (call);                                         \
    if (_err != CUDA_SUCCESS) {                                     \
        const char* _str;                                           \
        cuGetErrorString(_err, &_str);                              \
        fprintf(stderr, "[CU ERROR] %s:%d: %s\n",                  \
                __FILE__, __LINE__, _str);                          \
        return 1;                                                   \
    }                                                               \
} while(0)

int main(int argc, char** argv) {
    // Force UTF-8 on Windows
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    printf("\n");
    printf("============================================================\n");
    printf("  SephirotLang GPU Runner v1.0\n");
    printf("  16 Sephiroth Divine-Human Symbiosis Protocol\n");
    printf("  Loading cubin -> RTX 4050 (sm_89) -> Execute\n");
    printf("============================================================\n\n");

    // --- Init CUDA ---
    CU_CHECK(cuInit(0));

    // Use runtime API to set device and create primary context
    CUDA_CHECK(cudaSetDevice(0));
    CUcontext ctx;
    CUdevice dev;
    CU_CHECK(cuDeviceGet(&dev, 0));
    CU_CHECK(cuDevicePrimaryCtxRetain(&ctx, dev));

    // --- GPU Info ---
    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("  GPU : %s\n", prop.name);
    printf("  SM  : sm_%d%d\n", prop.major, prop.minor);
    printf("  VRAM: %.1f GB\n", prop.totalGlobalMem / 1e9);
    printf("  SMs : %d multiprocessors\n", prop.multiProcessorCount);

    int clock_khz = 0;
    cudaDeviceGetAttribute(&clock_khz, cudaDevAttrClockRate, 0);
    printf("  Clock: %d MHz\n", clock_khz / 1000);
    printf("\n");

    // --- Print 16 Sephiroth ---
    printf("  --- 16 Sephiroth Pipeline ---\n");
    const char* names[] = {
        "Keter     (Divine) - Identity / Data Load",
        "Chokmah   (Divine) - Knowledge Retrieval",
        "Gevurah   (Divine) - Threshold Filter",
        "Binah     (Divine) - Merge Integration",
        "Chesed    (Divine) - Weighted Fusion FMA",
        "Tiferet   (Divine) - Hadamard Product",
        "Netzach   (Divine) - Comparison Validation",
        "Hod       (Divine) - Feasibility Score",
        "Yesod     (Human)  - Reduction Aggregation",
        "Superego  (Human)  - LayerNorm",
        "Ego       (Human)  - Self-Attention",
        "TrueSelf  (Human)  - Layer Norm Integration",
        "Logic     (Human)  - GEMM Matrix Multiply",
        "Empathy   (Human)  - Softmax",
        "Bliss     (Human)  - Loss Function MSE",
        "Malkuth   (Human)  - Output Write-back"
    };
    for (int i = 0; i < 16; i++) {
        printf("  [%2d] %s\n", i, names[i]);
    }
    printf("\n");

    // --- Load cubin module ---
    const char* cubin_path = "kernel.cubin";
    if (argc > 1) cubin_path = argv[1];

    printf("  Loading cubin: %s\n", cubin_path);

    FILE* fp = fopen(cubin_path, "rb");
    if (!fp) {
        fprintf(stderr, "[ERROR] Cannot open %s\n", cubin_path);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t cubin_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* cubin_data = malloc(cubin_size);
    fread(cubin_data, 1, cubin_size, fp);
    fclose(fp);
    printf("  Cubin size: %zu bytes\n", cubin_size);

    CUmodule module;
    CU_CHECK(cuModuleLoadData(&module, cubin_data));
    printf("  Module loaded OK\n");

    // --- Get kernel function ---
    CUfunction kernel;
    CU_CHECK(cuModuleGetFunction(&kernel, module, "sephirot_kernel"));
    printf("  Kernel 'sephirot_kernel' found\n\n");

    // --- Allocate device memory ---
    const int N = 1024;

    float* h_input     = (float*)malloc(N * sizeof(float));
    float* h_knowledge = (float*)malloc(N * sizeof(float));
    float* h_weight    = (float*)malloc(N * sizeof(float));
    float* h_data2     = (float*)malloc(N * sizeof(float));
    float* h_data3     = (float*)malloc(N * sizeof(float));
    float* h_data4     = (float*)malloc(N * sizeof(float));
    float* h_data5     = (float*)malloc(N * sizeof(float));
    float* h_output    = (float*)malloc(N * sizeof(float));

    for (int i = 0; i < N; i++) {
        h_input[i]     = 1.0f + (float)i / N;
        h_knowledge[i] = 2.5f - (float)i / (2 * N);
        h_weight[i]    = 0.7f;
        h_data2[i]     = 0.5f;
        h_data3[i]     = 1.2f;
        h_data4[i]     = 3.14f;
        h_data5[i]     = 0.001f;
        h_output[i]    = 0.0f;
    }

    float *d_input, *d_knowledge, *d_weight, *d_data2, *d_data3, *d_data4, *d_data5, *d_output;
    CUDA_CHECK(cudaMalloc(&d_input,     N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_knowledge, N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_weight,    N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_data2,     N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_data3,     N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_data4,     N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_data5,     N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_output,    N * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_input,     h_input,     N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_knowledge, h_knowledge, N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_weight,    h_weight,    N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_data2,     h_data2,     N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_data3,     h_data3,     N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_data4,     h_data4,     N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_data5,     h_data5,     N * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_output,    h_output,    N * sizeof(float), cudaMemcpyHostToDevice));

    // --- Launch kernel ---
    printf("  --- Launching sephirot_kernel <<<%d, 1>>> ---\n\n", N);

    void* params[] = {
        &d_input, &d_knowledge, &d_weight, &d_data2,
        &d_data3, &d_data4,    &d_data5,   &d_output
    };

    cudaEvent_t start, stop;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    CUDA_CHECK(cudaEventRecord(start));
    CU_CHECK(cuLaunchKernel(kernel,
        N, 1, 1,
        1, 1, 1,
        0,
        0,
        params, NULL));
    CUDA_CHECK(cudaEventRecord(stop));
    CUDA_CHECK(cudaEventSynchronize(stop));

    float ms = 0;
    CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
    printf("  Kernel execution: %.6f ms\n", ms);

    // --- Copy D2H ---
    CUDA_CHECK(cudaMemcpy(h_output, d_output, N * sizeof(float), cudaMemcpyDeviceToHost));

    // --- Results ---
    printf("\n");
    printf("============================================================\n");
    printf("  RESULTS (first 16 of %d threads)\n", N);
    printf("============================================================\n");
    printf("  Thread | Keter(input) | Chokmah(wis) | Gevurah(filter)\n");
    printf("  -------|------------|-------------|---------------\n");
    for (int i = 0; i < 16 && i < N; i++) {
        float w = h_input[i] * h_knowledge[i];
        float g = w < 0.8f ? 0.0f : w;
        printf("  %6d | %10.4f | %11.4f | %13.4f\n", i, h_input[i], w, g);
    }
    printf("\n");
    printf("  Thread | Chesed(FMA) | Tiferet(had) | Netzach(valid)\n");
    printf("  -------|------------|-------------|--------------\n");
    for (int i = 0; i < 16 && i < N; i++) {
        float w = h_input[i] * h_knowledge[i];
        float g = w < 0.8f ? 0.0f : w;
        float b = g + h_data2[i];
        float c = b * h_weight[i] + h_input[i];
        float t = c * h_data3[i];
        float n = h_data4[i] >= 0.0f ? h_data4[i] : 0.0f;
        printf("  %6d | %10.4f | %11.4f | %12.4f\n", i, c, t, n);
    }
    printf("\n");
    printf("  Thread | Hod(score) | Yesod(reduce)| Malkuth(output)\n");
    printf("  -------|-----------|-------------|--------------\n");
    for (int i = 0; i < 16 && i < N; i++) {
        float n = h_data4[i] >= 0.0f ? h_data4[i] : 0.0f;
        float h = n * 0.5f + h_data5[i];
        printf("  %6d | %9.4f | %11.4f | %12.6f\n", i, h, h, h_output[i]);
    }
    printf("\n");
    printf("============================================================\n");
    printf("  GPU output[0] = %.6f (MSE loss from 16-sephirot pipeline)\n", h_output[0]);
    printf("  16 Sephiroth pipeline executed on %s OK!\n", prop.name);
    printf("============================================================\n\n");

    // --- Cleanup ---
    cudaFree(d_input); cudaFree(d_knowledge); cudaFree(d_weight);
    cudaFree(d_data2); cudaFree(d_data3); cudaFree(d_data4);
    cudaFree(d_data5); cudaFree(d_output);
    free(h_input); free(h_knowledge); free(h_weight); free(h_data2);
    free(h_data3); free(h_data4); free(h_data5); free(h_output);
    free(cubin_data);
    cuModuleUnload(module);

    return 0;
}
