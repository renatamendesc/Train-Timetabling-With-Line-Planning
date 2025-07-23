#pragma once
#if !defined(__CUDACC__)
#  define __global__
#  define __device__
#endif
#ifdef __cplusplus
extern "C" {
#endif
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
);
#ifdef __cplusplus
}
#endif