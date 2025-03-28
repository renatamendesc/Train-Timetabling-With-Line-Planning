#ifndef COMB_HPP
#define COMB_HPP

#include "Data.hpp"
#include "Model.hpp"
#include <cmath>
#include <algorithm>
#include <set>

class Combinations
{
public:
    unsigned long long int total_nb_combinations = 0;
    unsigned long long int nb_feasible_combinations = 0;

    Combinations(Data &data);

private:
    unsigned long long int max_nb_trips_combinations = 0;

    int max_nb_trips;
    int nb_routes;

    int nb_trains;

    std::set<std::vector<std::vector<int>>> prohibited_combinations_full;
    std::set<std::vector<int>> prohibited_combinations_trips;
    std::set<std::vector<int>> unique_combinations;

    std::vector<std::vector<int>> all_combinations;
    std::vector<std::vector<int>> trips_combinations;

    void reset_directory(Data &data);

    void generate_trips_combinations(Data &data);
    void generate_all_combinations(Data &data, Model &model);

    bool check_trips_feasibility (Data &data, std::vector <int> &current);
    bool check_final_feasibility (Data &data, std::vector <int> &current);

    void add_to_prohibited_set_full(std::vector <std::vector<int>> &invalid_combination);
    void add_to_prohibited_set_trips(std::vector <int> invalid_combination);
};

#endif