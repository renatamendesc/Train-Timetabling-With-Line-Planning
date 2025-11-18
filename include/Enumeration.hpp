#ifndef ENUMERATION_HPP
#define ENUMERATION_HPP

#include "Data.hpp"
#include "Model-OR-Tools.hpp"
#include "Combinations.hpp"
#include "Solution.hpp"

#include <omp.h>
#include <cmath>
#include <algorithm>
#include <set>

class Enumeration
{
    public:
        Solution overall_best_sol;
        Combinations comb;

        unsigned long long int total_nb_combinations = 0;
        unsigned long long int nb_feasible_combinations = 0;

        Enumeration(Data &data, int nb_threads, int time_limit_complete, int time_limit_per_combination);

    private:
        std::chrono::time_point<std::chrono::steady_clock> start;
        std::chrono::time_point<std::chrono::steady_clock> end;

        int time_limit_complete;
        int time_limit_per_combination;
        
        int nb_threads;
        int nb_trains;

        void execute_enumeration(Data &data);
        void execute_all_combinations(Data &data);

        void reached_time_limit(Data &data, std::chrono::duration<double> time);
};

#endif