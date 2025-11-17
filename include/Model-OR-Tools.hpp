#ifndef MODEL_OR_TOOLS_HPP
#define MODEL_OR_TOOLS_HPP

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "ortools/base/init_google.h"
#include "ortools/init/init.h"
#include "ortools/linear_solver/linear_solver.h"

#include "Data.hpp"
#include "Solution.hpp"

#include <vector>
#include <iostream>
#include <memory>
#include <chrono>

#define BIG_M 100000

typedef std::vector<std::vector<operations_research::MPVariable*>> NumVarMatrix2d;
typedef std::vector<NumVarMatrix2d> NumVarMatrix3d;
typedef std::vector<NumVarMatrix3d> NumVarMatrix4d;
typedef std::vector<NumVarMatrix4d> NumVarMatrix5d;
typedef std::vector<NumVarMatrix5d> NumVarMatrix6d;

class ModelORTools
{
    public:
        Solution best_sol;
        Solution current_sol;

        void initialize(Data &data, int threads);

        void create_full_model(Data &data);
        void create_model_with_routes_constraints(Data &data, std::vector<std::vector<int>> &routes_of_trains);

        void add_variables(Data &data);
        void add_constraints(Data &data);

        int execute_solver_for_full_model(Data &data);
        int execute_solver_for_combination(Data &data, int best_bound, int time_limit_for_combination);

        void get_value_of_variables(Data &data);

        // ~ModelORTools();

    private:
        int nb_threads;

        std::unique_ptr<operations_research::MPSolver> solver;
        operations_research::MPObjective* objective;

        // variables
        NumVarMatrix3d x_;
        NumVarMatrix4d x_bar_;
        NumVarMatrix3d y_;
        NumVarMatrix3d y_bar_;
        NumVarMatrix3d lambda_;
        NumVarMatrix6d w_;
        NumVarMatrix5d u_;
        operations_research::MPVariable* z_;
};

#endif