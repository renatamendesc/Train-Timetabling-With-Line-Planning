#ifndef HEURISTIC_HPP
#define HEURISTIC_HPP

#include "Data.hpp"
#include <cmath>

class Heuristic
{
public:
    void create_initial_combinations (Data &data);
    void remove_trips (Data &data, bool feasible, std::vector<std::vector<int>> current);
    std::vector<std::vector<std::vector<int>>> candidate_combinations;

private:
    std::vector <int> cyclical_routes_set;
    void create_cyclical_routes_set(Data &data);
    std::vector <int> initial_valid_routes_set;
    void create_initial_valid_routes_set(Data &data);

    void add_all_subsets (Data &data);

};

#endif