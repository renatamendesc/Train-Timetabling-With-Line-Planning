#ifndef MODEL_HPP
#define MODEL_HPP

#include "Data.hpp"
#include <ctime>
#include <chrono>
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

struct Solution {
    double obj_value;
    double gap_value;
    double computational_time;

    VarValuesMatrix3d y_values;
    VarValuesMatrix3d y_bar_values;
    VarValuesMatrix3d lambda_values;
};

class Model
{
public:
    Solution sol;

    void init (Data &data);

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

    int extract_solution(Data &data);

    void get_value_of_variables(Data &data, IloCplex &cplex);
    void get_final_solution(Data &data);

    std::string convert_time(int seconds);

};

#endif
