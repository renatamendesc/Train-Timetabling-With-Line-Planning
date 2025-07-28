#ifndef HEURISTIC_HPP
#define HEURISTIC_HPP

#include "Data.hpp"
#include <cmath>
#include <algorithm>
#include <set>

class Heuristic
{
public:
    std::vector<std::vector<std::vector<int>>> candidate_combinations;

    void create_initial_candidates (Data &data);
    bool remove_trips (Data &data, bool remove_from_all_candiates, std::vector<std::vector<int>> &current, int iter);
    bool add_trips (Data &data);
    void change_trips (Data &data, bool change_all_candidates, std::vector<std::vector<int>> &current);
    
    void create_subsets (Data &data, std::vector<std::vector<std::vector<int>>> &aux_candidates, std::vector<std::vector<int>> &current, int max_nb_trips);

    bool have_cycles = false;

    void try_new_set_of_routes(Data &data, int iter_set);

private:
    std::vector<std::vector<int>> cycles_of_routes;

    std::vector <int> cyclical_routes_set;
    void create_cyclical_routes_set(Data &data);
    std::vector <int> initial_valid_routes_set;
    void create_initial_valid_routes_set(Data &data);
    // void create_artificial_cycles(Data &data);

    // create new set of selected routes

    void create_maximum_size_candidates(Data &data);
    void add_trips_till_demands_are_met(Data &data, int nb_trains, int num_valid, int num_cyclic);

    bool verify_demands(Data &data, std::vector<std::vector<int>> &current);
    bool verify_compatibility (Data &data, std::vector<int> &current);
    bool normalize_candidates (Data &data, std::vector<std::vector<int>> &current);
    std::set<std::vector<std::vector<int>>> unique_combinations;

};

#endif