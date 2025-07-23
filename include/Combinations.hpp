#ifndef COMB_HPP
#define COMB_HPP

#include "Data.hpp"
#include "Model.hpp"
#include "Heuristic.hpp"
#include <omp.h>
#include <cmath>
#include <algorithm>
#include <set>
#include <iomanip>

class Combinations
{
public:
    unsigned long long int total_nb_combinations = 0;
    unsigned long long int nb_feasible_combinations = 0;

    Combinations(Data &data, int threads, int strategy);

private:
    Model best_thread;
    Heuristic heuristic;

    // *tamanhos* que você usava no CUDA
    int max_routes_per_train;
    int total_routes;
    int total_flat;
    int nb_vertices;
    unsigned long long aux_progress;

    // *arrays host* lineares para mandar à GPU
    int* host_trip_route_idx = nullptr;    // já preenchido antes
    int* host_route_lengths  = nullptr;
    int* host_route_offsets  = nullptr;
    int* host_flat_routes    = nullptr;

    // *ponteiros device*
    int* d_trips_sizes       = nullptr;
    int* d_trip_route_idx    = nullptr;
    int* d_route_lengths     = nullptr;
    int* d_route_offsets     = nullptr;
    int* d_flat_routes       = nullptr;
    int* d_demand_per_day    = nullptr;

    int best_bound = __INT_MAX__; // best integer solution found

    int nb_threads;
    int nb_routes; 
    int nb_trains;

    int counter_solved = 0;

    void execute_enumeration(Data &data);
    void execute_heuristic (Data &data);

    void execute_all_combinations_hybrid(Data &data);
    void execute_all_combinations(Data &data);
    void execute_candidate_combinations (Data &data);

    std::set<std::vector<std::vector<int>>> unique_combinations;
    std::vector<std::vector<std::vector<int>>> trips_combinations;

    void generate_trips_combinations(Data &data, int train_idx);
    unsigned long long int max_nb_trips_combinations = 0;

    bool normalize_combination (Data &data, std::vector<std::vector<int>> &current);
    bool check_trips_feasibility (Data &data, std::vector<int> &current);
    bool check_final_feasibility (Data &data, std::vector<std::vector<int>> &current);

    bool verify_overflow(unsigned long long base, unsigned long long exp);
};

#endif