#ifndef MODEL_OR_TOOLS_HPP
#define MODEL_OR_TOOLS_HPP

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "ortools/base/init_google.h"
#include "ortools/init/init.h"
#include "ortools/linear_solver/linear_solver.h"

#include "Data.hpp"
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

typedef std::vector<std::vector<int>> VarValuesMatrix2d;
typedef std::vector<VarValuesMatrix2d> VarValuesMatrix3d;
typedef std::vector<VarValuesMatrix3d> VarValuesMatrix4d;
typedef std::vector<VarValuesMatrix4d> VarValuesMatrix5d;
typedef std::vector<VarValuesMatrix5d> VarValuesMatrix6d;

class ModelORTools
{
    public:
        void initialize(Data &data, int threads);
        void add_variables(Data &data);
        void add_constraints(Data &data);

        int execute_solver(Data &data);

        void get_value_of_variables(Data &data);
        void get_solution(Data &data);

    private:
        std::chrono::duration<double> time;

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

        // values of variables
        VarValuesMatrix3d x_values_;
        VarValuesMatrix4d x_bar_values_;
        VarValuesMatrix3d y_values_;
        VarValuesMatrix3d y_bar_values_;
        VarValuesMatrix3d lambda_values_;
        VarValuesMatrix6d w_values_;
        VarValuesMatrix5d u_values_;

        std::string convert_time(int seconds);
};

#endif