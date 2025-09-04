#ifndef MODEL_HPP
#define MODEL_HPP

#include "Data.hpp"
#include <omp.h>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <ilcplex/ilocplex.h>

#define BIG_M 100000

typedef IloArray<IloNumVarArray> NumVarMatrix2d;
typedef IloArray<NumVarMatrix2d> NumVarMatrix3d;
typedef IloArray<NumVarMatrix3d> NumVarMatrix4d;
typedef IloArray<NumVarMatrix4d> NumVarMatrix5d;
typedef IloArray<NumVarMatrix5d> NumVarMatrix6d;

typedef std::vector<std::vector<int>> VarValuesMatrix2d;
typedef std::vector<VarValuesMatrix2d> VarValuesMatrix3d;
typedef std::vector<VarValuesMatrix3d> VarValuesMatrix4d;
typedef std::vector<VarValuesMatrix4d> VarValuesMatrix5d;
typedef std::vector<VarValuesMatrix5d> VarValuesMatrix6d;

struct Solution
{
    int obj_value = __INT_MAX__;
    double gap_value;
    double computational_time;

    std::chrono::steady_clock::time_point time_found;

    VarValuesMatrix3d y_values;
    VarValuesMatrix3d y_bar_values;
    VarValuesMatrix3d lambda_values;
};

class Model
{
public:

    bool verify_feasibility = false;

    std::vector<Solution> best_sol;
    Solution current_sol;

    void initialize (Data &data);
    void reset (Data &data);

    int run (Data &data);
    int run_with_routes_constraints (Data &data, std::vector<std::vector<int>> &routes_of_trains, int best_bound, int time_limit);

    void tie_breaker(Data &data, std::vector<std::vector<std::vector<int>>> &combinations);

    void get_solution (Data &data, bool is_final_solution, bool print_gap);
    void get_best_combinations (Data &data, std::vector<std::vector<std::vector<int>>> &combination);
    void get_graph (Data &data);

private:
    IloEnv env;
    IloModel model;
    IloConstraintArray constraints;
    IloExpr obj;

    NumVarMatrix3d x_;
    NumVarMatrix4d x_bar_;
    NumVarMatrix3d y_;
    NumVarMatrix3d y_bar;
    NumVarMatrix3d lambda_;
    NumVarMatrix2d beta_;
    IloNumVar z_;
    NumVarMatrix6d w_;
    NumVarMatrix5d u_;

    void add_variables (Data &data);
    void add_constraints (Data &data);

    int extract_solution (Data &data, bool is_final_solution, int best_bound, int time_limit);

    void get_value_of_variables (Data &data, IloCplex &cplex, bool is_final_solution);

    std::string convert_time (int seconds);
};

#endif
