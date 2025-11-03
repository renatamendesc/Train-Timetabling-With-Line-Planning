#include "Model-OR-Tools.hpp"
#include <iostream>
#include <memory>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/log.h"
#include "ortools/base/init_google.h"
#include "ortools/init/init.h"
#include "ortools/linear_solver/linear_solver.h"

using namespace operations_research;
using namespace std;

// ------------------------------------------------------------------------------------------- //
// Input: LD_LIBRARY_PATH=/home/renata/or-tools/build/lib:$LD_LIBRARY_PATH ./cbtu <instance>
// ------------------------------------------------------------------------------------------- //

void ModelORTools::initialize(Data &data, int threads)
{
    // create the linear solver with the HIGHS
    auto solver(MPSolver::CreateSolver("HIGHS"));
    if (!solver)
    {
        cerr << "Warning: Could not create solver HIGHS" << endl;
        return;
    }

    // create the model
    add_variables(data);
    add_constraints(data);

    return;
}

void ModelORTools::add_variables(Data &data)
{
    // create variable x - specifies whether train t on trip i uses arc a
    x_ = NumVarMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_[t] = NumVarMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_[t][i] = NumVarMatrix2d(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                string name = "x(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")";
                x_[t][i][a] = solver->MakeBoolVar(name.c_str());
            }
        }
    }

    // create variable x bar - specifies whether train t on trip i uses arc a on time interval h
    x_bar_ = NumVarMatrix4d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_bar_[t] = NumVarMatrix3d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_bar_[t][i] = NumVarMatrix2d(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                x_bar_[t][i][a] = NumVarMatrix2d(data.get_nb_intervals());
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    string name = "x_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")(" + to_string(h) + ")";
                    x_bar_[t][i][a][h] = solver->MakeBoolVar(name.c_str());
                }
            }
        }
    }

    // create variable y - departure time at a vertex
    y_ = NumVarMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_[t] = NumVarMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_[t][i] = NumVarMatrix2d(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                string name = "y(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                y_[t][i][v] = solver->MakeNumVar(0, IloInfinity, name.c_str());
            }
        }
    }

    // create variable y bar - arrival time at a vertex
    y_bar_ = NumVarMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_bar_[t] = NumVarMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_bar_[t][i] = NumVarMatrix2d(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                string name = "y_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                y_bar_[t][i][v] = solver->MakeNumVar(0, IloInfinity, name.c_str());
            }
        }
    }

    // create variable lambda - specifies whether train t on trip i uses route r
    lambda_ = NumVarMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        lambda_[t] = NumVarMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            lambda_[t][i] = NumVarMatrix2d(data.get_nb_routes());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                string name = "lambda(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                lambda_[t][i][r] = solver->MakeBoolVar(name.c_str());
            }
        }
    }

    // create variable w - specifies whether train t departs from vertex v after train l departs from vertex k
    w_ = NumVarMatrix6d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        w_[t] = NumVarMatrix5d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            w_[t][i] = NumVarMatrix4d(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                w_[t][i][v] = NumVarMatrix3d(data.get_nb_trains());
                for (int l = 0; l < data.get_nb_trains(); l++)
                {
                    if (t != l)
                    {
                        w_[t][i][v][l] = NumVarMatrix2d(data.get_train_max_trips(l));
                        for (int j = 0; j < data.get_train_max_trips(l); j++)
                        {
                            for (int k = 0; k < data.get_nb_vertices(); k++)
                            {
                                for (auto inc_point : data.get_inc_points())
                                {
                                    int k_aux = get<0>(inc_point);
                                    int v_aux = get<2>(inc_point);

                                    if (v_aux == v && k_aux == k)
                                    {
                                        string name = "w(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")";
                                        w_[t][i][v][l][j][k] = solver->MakeBoolVar(name.c_str());
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // create variable u - specifies whether train t departs from vertex v after train l
    u_ = NumVarMatrix5d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        u_[t] = NumVarMatrix4d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            u_[t][i] = NumVarMatrix3d(data.get_nb_trains());
            for (int l = 0; l < data.get_nb_trains(); l++)
            {
                if (t != l)
                {
                    u_[t][i][l] = NumVarMatrix2d(data.get_train_max_trips(l));
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        u_[t][i][l][j] = NumVarMatrix2d(data.get_nb_vertices());
                        for (int v = 0; v < data.get_nb_vertices(); v++)
                        {
                            string name = "u(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            u_[t][i][l][j][v] = solver->MakeBoolVar(name.c_str());
                        }
                    }
                }
            }
        }
    }

    // create variable z - objective function
    z_ = solver->MakeNumVar(0, IloInfinity, "z");
}

void ModelORTools::add_constraints(Data &data)
{

}