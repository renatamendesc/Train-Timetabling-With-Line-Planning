#ifndef SOLUTION_HPP
#define SOLUTION_HPP

#include "Data.hpp"
#include "Combinations.hpp"

#include <chrono>
#include <vector>

typedef std::vector<std::vector<int>> VarValuesMatrix2d;
typedef std::vector<VarValuesMatrix2d> VarValuesMatrix3d;
typedef std::vector<VarValuesMatrix3d> VarValuesMatrix4d;
typedef std::vector<VarValuesMatrix4d> VarValuesMatrix5d;
typedef std::vector<VarValuesMatrix5d> VarValuesMatrix6d;

class Solution
{
    public:
        int obj_value = __INT_MAX__;
        std::chrono::duration<double> computational_time;
        double gap_value;
        
        bool proven_optimal = true;
        bool feasible = false;

        // values of variables
        VarValuesMatrix3d x_values_;
        VarValuesMatrix4d x_bar_values_;
        VarValuesMatrix3d y_values_;
        VarValuesMatrix3d y_bar_values_;
        VarValuesMatrix3d lambda_values_;
        VarValuesMatrix6d w_values_;
        VarValuesMatrix5d u_values_;

        RoutesCombination routes_combination;
        void store_combination(Data &data);

        int max_nb_repeated_routes;
        void store_max_nb_repeated_route(Data &data);

        void display_routes_combination(Data &data);
        void display_solution(Data &data);
        void display_variables_values(Data &data);

        void create_graph(Data &data, std::string method, int nb_threads);
    
    private:
        std::string convert_time(int seconds);
};

#endif