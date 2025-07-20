#ifndef HEURISTIC_HPP
#define HEURISTIC_HPP

#include "Data.hpp"
#include <cmath>

class Heuristic
{
public:
    std::vector<std::vector<std::vector<int>>> candidate_combinations;

    void create_initial_candidates (Data &data);
    bool add_trips (Data &data);
    void change_trips (Data &data, std::vector<std::vector<int>> &current);
    void create_subsets (Data &data, int iter);

    bool found_new_feasible_combination = false;

private:
    std::vector <int> cyclical_routes_set;
    void create_cyclical_routes_set(Data &data);
    std::vector <int> initial_valid_routes_set;
    void create_initial_valid_routes_set(Data &data);

    // create new set of selected routes
    void try_new_set_of_routes(Data &data);

    void create_maximum_size_candidates(Data &data, int nb_trains, int num_valid, int num_cyclic);
    void add_trips_till_demands_are_met(Data &data, int nb_trains, int num_valid, int num_cyclic);

    bool verify_demands(Data &data, std::vector<std::vector<int>> &current);
    bool verify_compatibility (Data &data, std::vector<int> &current);
};

#endif