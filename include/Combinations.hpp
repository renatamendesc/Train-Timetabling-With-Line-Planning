#ifndef COMB_HPP
#define COMB_HPP

#include "Data.hpp"
#include <cmath>

class Combinations
{

public:
    unsigned long long int total_nb_combinations = 0;
    unsigned long long int nb_trips_combinations = 0;

    int max_nb_trips;
    int nb_routes;

    int nb_trains;

    std::vector<std::vector<int>> all_combinations;
    std::vector<std::vector<int>> trips_combinations;

    void init(Data &data);

private:
    void generate_trips_combinations();
    void generate_all_combinations();

};

#endif