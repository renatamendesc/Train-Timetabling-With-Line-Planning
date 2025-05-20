#ifndef COMB_HPP
#define COMB_HPP

#include "Data.hpp"
#include "Model.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <queue>
#include <thread>
#include <mutex>
#include <semaphore.h>

class Combinations
{
public:
    unsigned long long int total_nb_combinations = 0;
    unsigned long long int nb_feasible_combinations = 0;

    Combinations(Data &data, int t, std::string scheduling, int strat);

private:
    unsigned long long int max_nb_trips_combinations = 0;

    int max_nb_trips;
    int nb_routes;
    int nb_effective_routes; // used when we consider nb_routes+1 as route not made

    int nb_trains;

    std::set<std::vector<std::vector<int>>> prohibited_combinations_full;
    std::set<std::vector<int>> prohibited_combinations_trips;
    std::set<std::vector<std::vector<int>>> unique_combinations;

    // std::vector<std::vector<int>> all_combinations;
    std::vector<std::vector<std::vector<int>>> trips_combinations;

    std::queue<std::pair<int, int>> queue_chunks;

    void reset_directory(Data &data);
    bool verify_overflow(unsigned long long base, unsigned long long exp);
    
    void worker (Data &data, int thread_id);

    void generate_trips_combinations(Data &data, int train_idx);
    void generate_all_combinations(Data &data, unsigned long long int start, unsigned long long int end, int thread_id);

    bool check_trips_feasibility (Data &data, std::vector<int> &current);
    bool check_final_feasibility (Data &data, std::vector<std::vector<int>> &current);

    int strategy;

    int nb_threads;
    Model best_thread;
    std::mutex mtx;
    
    int aux_progress;
    int counter_solved = 0;
};

#endif