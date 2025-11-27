#ifndef HEURISTIC_HPP
#define HEURISTIC_HPP

#include "Data.hpp"
#include "Model-OR-Tools.hpp"
#include "Combinations.hpp"
#include "Solution.hpp"

#include <omp.h>
#include <cmath>
#include <algorithm>
#include <set>

class Heuristic
{
    public:
        Solution overall_best_sol;

        Heuristic(Data &data, int nb_threads, int time_limit_complete, int time_limit_per_combination);

    private:
        Combinations comb;
        std::vector<RoutesCombination> candidate_combinations;

        int time_limit_complete;
        int time_limit_per_combination;

        int nb_threads;

        int iter = 0;
        bool improved_sol;

        bool have_cycles = false;

        void execute_heuristic(Data &data);

        std::vector <int> cyclical_routes_set;
        std::vector <int> initial_valid_routes_set;
        void create_initial_candidates (Data &data);
        void create_cyclical_routes_set(Data &data);
        void create_initial_valid_routes_set(Data &data);
        void create_maximum_size_candidates(Data &data);

        void execute_candidate_combinations(Data &data);

        void solve_initial_candidates(Data &data);

        void remove_trips (Data &data, std::vector<RoutesCombination> &new_candidates, RoutesCombination &current_combination, int max_nb_trips);
        bool create_subsets_from_best_combination(Data &data);
        bool create_subsets_from_all_candidates(Data &data);
        
        void try_new_set_of_routes(Data &data);

        void unfix_trips_from_best_combination (Data &data);
        void unfix_trips_from_all_candidates (Data &data);
        
        std::chrono::time_point<std::chrono::steady_clock> start;
        std::chrono::time_point<std::chrono::steady_clock> end;
        void reached_time_limit(Data &data, std::chrono::duration<double> time);

        void display_candidates_combinations();

        // ===================================================================== //
        // (provavelmente retirar esses)
        bool verify_demands(Data &data, std::vector<std::vector<int>> &current);
        bool verify_compatibility (Data &data, std::vector<int> &current);
        bool normalize_candidates (Data &data, std::vector<std::vector<int>> &current);
        std::set<std::vector<std::vector<int>>> unique_combinations;
};

#endif