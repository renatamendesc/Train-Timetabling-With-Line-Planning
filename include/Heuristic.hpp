#ifndef HEURISTIC_HPP
#define HEURISTIC_HPP

#include "Data.hpp"
#include <cmath>

class Heuristic
{
public:
    std::vector<std::vector<std::vector<int>>> candidate_combinations;

    void create_initial_candidates (Data &data);
    bool remove_trips (Data &data, bool feasible, std::vector<std::vector<int>> &current, int min_nb_trips);
    void change_trips (Data &data, std::vector<std::vector<int>> &current);
    void create_subsets (Data &data, int iter);

private:
    std::vector <int> cyclical_routes_set;
    void create_cyclical_routes_set(Data &data);
    std::vector <int> initial_valid_routes_set;
    void create_initial_valid_routes_set(Data &data);

    bool verify_demands(Data &data, std::vector<std::vector<int>> &current);
    bool verify_compatibility (Data &data, std::vector<int> &current);
};

#endif