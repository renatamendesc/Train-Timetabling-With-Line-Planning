#ifndef COMB_HPP
#define COMB_HPP

#include "Data.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <iomanip>
#include <climits>

typedef std::vector<std::vector<int>> TripsCombination;
typedef std::vector<std::vector<int>> RoutesCombination;

class Combinations
{
    public:
        std::vector<TripsCombination> all_trips_combinations;

        int calculate_trips_combinations(Data &data);
        bool is_valid_combination (Data &data, RoutesCombination &combination);
        bool normalize_combination (Data &data, RoutesCombination &combination);

    private:
        std::set<std::vector<std::vector<int>>> unique_combinations;

        void generate_trips_combinations(Data &data, int train_idx);
        bool check_trips_feasibility (Data &data, std::vector<int> &sequence_of_trips);

        bool verify_overflow(unsigned long long base, unsigned long long exp);
};

#endif