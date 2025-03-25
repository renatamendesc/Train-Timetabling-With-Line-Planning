#ifndef COMB_HPP
#define COMB_HPP

#include "Data.hpp"
#include <cmath>
#include <algorithm>
#include <set>

class Combinations
{
public:
    unsigned long long int total_nb_combinations = 0;

    void init(Data &data);

private:
    unsigned long long int max_nb_trips_combinations = 0;

    int max_nb_trips;
    int nb_routes;

    int nb_trains;

    std::set<std::vector<int>> unique_combinations;

    std::vector<std::vector<int>> all_combinations;
    std::vector<std::vector<int>> trips_combinations;

    void generate_trips_combinations(Data &data);
    void generate_all_combinations(Data &data);

    bool check_trips_feasibility (Data &data, std::vector <int> &current);
    bool check_final_feasibility (Data &data, std::vector <int> &current);
};

#endif