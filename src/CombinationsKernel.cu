// src/CombinationsKernel.cu
#include "CombinationsKernel.cuh"
#include <cuda_runtime.h>

// device function
__device__ bool check_final_feasibility_gpu(
    const int* route,
    int         len,
    const int*  demand_per_day,
    int         nb_vertices
) {
    for (int i = 0; i < len; ++i) {
        int v = route[i];
        if (v < 0 || v >= nb_vertices) 
            return false;
    }
    for (int i = 0; i < len; ++i) {
        if (demand_per_day[ route[i] ] > 0)
            return true;
    }
    return false;
}

// kernel
extern "C"
__global__ void processCombinationsKernel(
    const int*    d_trips_sizes,
    const int*    d_trip_route_idx,
    const int*    d_route_lengths,
    const int*    d_route_offsets,
    const int*    d_flat_routes,
    const int*    d_demand_per_day,
    int           nb_vertices,
    int           nb_trains,
    int           max_routes_per_train,
    unsigned long long total_nb_combinations,
    unsigned long long start_idx,
    bool*         d_feasibleFlags
) {
    auto local_id = blockIdx.x*(unsigned long long)blockDim.x + threadIdx.x;
    auto gid      = start_idx + local_id;
    if (local_id >= gridDim.x*(unsigned long long)blockDim.x || gid >= total_nb_combinations)
        return;

    extern __shared__ int s_idx[];
    auto code = gid;
    for (int k = nb_trains - 1; k >= 0; --k) {
        int sz   = d_trips_sizes[k];
        s_idx[k] = code % sz;
        code    /= sz;
    }

    bool feas = true;
    for (int k = 0; k < nb_trains; ++k) {
        int combo_j          = s_idx[k];
        int global_route_idx = d_trip_route_idx[k*max_routes_per_train + combo_j];
        int len              = d_route_lengths[global_route_idx];
        int offs             = d_route_offsets[global_route_idx];
        const int* route_ptr = &d_flat_routes[offs];

        feas = feas && check_final_feasibility_gpu(
            route_ptr, len,
            d_demand_per_day, nb_vertices
        );
        if (!feas) break;
    }
    d_feasibleFlags[local_id] = feas;
}
