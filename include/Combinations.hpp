#ifndef COMB_HPP
#define COMB_HPP

#include "Data.hpp"
#include "Model.hpp"
#include "Heuristic.hpp"
#include <omp.h>
#include <cmath>
#include <algorithm>
#include <set>
#include <atomic>
#include <iomanip>

class Combinations
{
    public:
        unsigned long long int total_nb_combinations = 0;
        unsigned long long int nb_feasible_combinations = 0;

        Combinations(Data &data, int threads, std::string method, int time_limit_complete = 43200);

    private:

        std::chrono::time_point<std::chrono::steady_clock> start;
        std::chrono::time_point<std::chrono::steady_clock> end;

        int time_limit_per_combination;
        int time_limit_complete = 43200;

        bool proved_optimal = true;

        Model best_thread;
        Heuristic heuristic;

        int best_bound = __INT_MAX__; // best integer solution found

        int nb_threads;
        int nb_routes; 
        int nb_trains;

        int aux_progress;
        int counter_solved = 0;

        void calculate_trips_combinations(Data &data);

        void execute_enumeration(Data &data);
        void execute_heuristic (Data &data);

        void execute_all_combinations(Data &data);
        bool execute_candidate_combinations (Data &data);

        std::set<std::vector<std::vector<int>>> unique_combinations;
        std::vector<std::vector<std::vector<int>>> trips_combinations;

        void generate_trips_combinations(Data &data, int train_idx);
        unsigned long long int max_nb_trips_combinations = 0;

        bool normalize_combination (Data &data, std::vector<std::vector<int>> &current);
        bool check_trips_feasibility (Data &data, std::vector<int> &current);
        bool is_valid_combiantion (Data &data, std::vector<std::vector<int>> &current);

        bool verify_overflow(unsigned long long base, unsigned long long exp);

        std::atomic<bool> stop_execution = false;
};

#endif