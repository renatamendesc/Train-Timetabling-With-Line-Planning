#include "Model-OR-Tools.hpp"

using namespace operations_research;
using namespace std;

// ------------------------------------------------------------------------------------------- //
// Input: LD_LIBRARY_PATH=/home/renata/or-tools/build/lib:$LD_LIBRARY_PATH ./cbtu <instance>
// ------------------------------------------------------------------------------------------- //

void ModelORTools::initialize(Data &data, int threads)
{
    // create the linear solver with the HIGHS
    solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("HIGHS"));
    if (!solver)
    {
        cerr << "Warning: Could not create solver HIGHS" << endl;
        return;
    }

    // create the model
    cout << endl << "Creating model..." << endl;

    add_variables(data);

    cout << "Creating objective function..." << endl;   
    objective = solver->MutableObjective();
    objective->SetCoefficient(z_, 1);
    objective->SetMinimization();

    add_constraints(data);

    return;
}

void ModelORTools::add_variables(Data &data)
{

    cout << "Creating variables..." << endl;

    // create variable x - specifies whether train t on trip i uses arc a
    x_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_[t][i].resize(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                string name = "x(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")";
                x_[t][i][a] = solver->MakeBoolVar(name.c_str());
            }
        }
    }

    // create variable x bar - specifies whether train t on trip i uses arc a on time interval h
    x_bar_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_bar_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_bar_[t][i].resize(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                x_bar_[t][i][a].resize(data.get_nb_intervals());
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    string name = "x_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")(" + to_string(h) + ")";
                    x_bar_[t][i][a][h] = solver->MakeBoolVar(name.c_str());
                }
            }
        }
    }

    // create variable y - departure time at a vertex
    y_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_[t][i].resize(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                string name = "y(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                y_[t][i][v] = solver->MakeNumVar(0, solver->infinity(), name.c_str());
            }
        }
    }

    // create variable y bar - arrival time at a vertex
    y_bar_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_bar_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_bar_[t][i].resize(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                string name = "y_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                y_bar_[t][i][v] = solver->MakeNumVar(0, solver->infinity(), name.c_str());
            }
        }
    }

    // create variable lambda - specifies whether train t on trip i uses route r
    lambda_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        lambda_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            lambda_[t][i].resize(data.get_nb_routes());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                string name = "lambda(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                lambda_[t][i][r] = solver->MakeBoolVar(name.c_str());
            }
        }
    }

    // create variable w - specifies whether train t departs from vertex v after train l departs from vertex k
    w_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        w_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            w_[t][i].resize(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                w_[t][i][v].resize(data.get_nb_trains());
                for (int l = 0; l < data.get_nb_trains(); l++)
                {
                    if (t != l)
                    {
                        w_[t][i][v][l].resize(data.get_train_max_trips(l));
                        for (int j = 0; j < data.get_train_max_trips(l); j++)
                        {
                            w_[t][i][v][l][j].resize(data.get_nb_vertices());
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
    u_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        u_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            u_[t][i].resize(data.get_nb_trains());
            for (int l = 0; l < data.get_nb_trains(); l++)
            {
                if (t != l)
                {
                    u_[t][i][l].resize(data.get_train_max_trips(l));
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        u_[t][i][l][j].resize(data.get_nb_vertices());
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
    z_ = solver->MakeNumVar(0, solver->infinity(), "z");
}

void ModelORTools::add_constraints(Data &data)
{
    cout << "Creating constraints..." << endl;

    // constraints to get value of z (2)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int l = 0; l < data.get_nb_trains(); l++)
            {
                for (int j = 0; j < data.get_train_max_trips(l); j++)
                {
                    for (int p = 0; p < data.get_nb_points(); p++)
                    {
                        if (data.is_station(p))
                        {
                            int v = data.get_point_vertices(p)[0];

                            string name = "max_gap_between_departures_upper(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            MPConstraint* c_1 = solver->MakeRowConstraint(-solver->infinity(), 0, name.c_str());
                            c_1->SetCoefficient(z_, -1);
                            c_1->SetCoefficient(y_[t][i][v], 1);
                            c_1->SetCoefficient(y_[l][j][v], -1);

                            v = data.get_point_vertices(p)[1];

                            name = "max_gap_between_departures_lower(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            c_1 = solver->MakeRowConstraint(-solver->infinity(), 0, name.c_str());
                            c_1->SetCoefficient(z_, -1);
                            c_1->SetCoefficient(y_[t][i][v], 1);
                            c_1->SetCoefficient(y_[l][j][v], -1);
                        }
                    }
                }
            }
        }
    }

    
    // associate x variable with x_bar variable (3)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {

                string name = "associate_x_and_x_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")";
                MPConstraint* c_2 = solver->MakeRowConstraint(0, 0, name.c_str());
                c_2->SetCoefficient(x_[t][i][a], -1);
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    c_2->SetCoefficient(x_bar_[t][i][a][h], 1);
                }
            }
        }
    }

    // constraints to associate routes with arcs (lambda variables with x variables) (4)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                string name = "associate_routes_with_arcs(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")";
                MPConstraint* c_3 = solver->MakeRowConstraint(0, 0, name.c_str());
                c_3->SetCoefficient(x_[t][i][a], -1);
                for (int r = 0; r < data.get_nb_routes(); r++)
                {
                    if (data.is_valid_route(t, i, r) && data.arc_belongs_to_route(r, a))
                    {
                        c_3->SetCoefficient(lambda_[t][i][r], 1);
                    }
                }
            }
        }
    }

    // each trip completed must be associated with at most one route (5)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            string name = "associate_trip_with_route(" + to_string(t) + ")(" + to_string(i) + ")";
            MPConstraint* c_4 = solver->MakeRowConstraint(-solver->infinity(), 1, name.c_str());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    c_4->SetCoefficient(lambda_[t][i][r], 1);
                }
            }
        }
    }

    // establish incompatible routes (6)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 1; i < data.get_train_max_trips(t); i++)
        {
            for (int r1 = 0; r1 < data.get_nb_routes(); r1++)
            {
                if (data.is_valid_route(t, i, r1))
                {
                    for (int r2 = 0; r2 < data.get_nb_routes(); r2++)
                    {
                        if (data.is_valid_route(t, i - 1, r2))
                        {
                            if (data.are_incompatible_routes(r2, r1))
                            {
                                string name = "incompatible_routes(" + to_string(t) + ")(" + to_string(i - 1) + ")(" + to_string(i) + ")(" + to_string(r1) + ")(" + to_string(r2) + ")";
                                MPConstraint* c_5 = solver->MakeRowConstraint(-solver->infinity(), 1, name.c_str());
                                c_5->SetCoefficient(lambda_[t][i][r1], 1);
                                c_5->SetCoefficient(lambda_[t][i - 1][r2], 1);
                            }
                        }
                    }
                }
            }
        }
    }

    // constraints to establish subsequential trips according to indexes (7)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 1; i < data.get_train_max_trips(t); i++)
        {
            string name = "subseq_trips(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(i - 1) + ")";
            MPConstraint* c_6 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    c_6->SetCoefficient(lambda_[t][i][r], -1);
                }
            }
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i - 1, r))
                {
                    c_6->SetCoefficient(lambda_[t][i - 1][r], 1);
                }
            }
        }
    }

    // constraints to connect trips of the same train (trip can only start after the previous one has ended) (8)
    for (int t = 0; t < data.get_nb_trains(); ++t)
    {
        for (int i = 1; i < data.get_train_max_trips(t); ++i)
        {
            for (int v = 0; v < data.get_nb_vertices(); ++v)
            {
                for (int k = 0; k < data.get_nb_vertices(); ++k)
                {
                    if (data.can_be_adjacent_in_consecutive_trips(v, k))
                    {
                        // y_[t][i][k] >= y_bar[t][i - 1][v] - BIG_M * (2 - expr1 - expr2))
                        // 0 >= y_bar[t][i - 1][v] - BIG_M * (2 - expr1 - expr2)) - y_[t][i][k] >= -inf
                        // 0 >= y_bar[t][i - 1][v] - BIG_M * 2 + BIG_M * expr1 + + BIG_M * expr2 - y_[t][i][k] >= -inf
                        // 0 >= y_bar[t][i - 1][v] - BIG_M * 2 + BIG_M * (expr1 + expr2) - y_[t][i][k] >= -inf
                        // BIG_M * 2 >= y_bar[t][i - 1][v] + BIG_M * (expr1 + expr2) - y_[t][i][k] >= -inf

                        string name = "connect_trips(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(k) + ")";
                        MPConstraint* c_7 = solver->MakeRowConstraint(-solver->infinity(), 2*BIG_M, name.c_str());
                        c_7->SetCoefficient(y_bar_[t][i - 1][v], 1);
                        c_7->SetCoefficient(y_[t][i][k], -1);
                        for (int r = 0; r < data.get_nb_routes(); r++)
                        {
                            if (data.finish_at_vertex(r, v) && data.is_valid_route(t, i - 1, r))
                            {
                                c_7->SetCoefficient(lambda_[t][i - 1][r], BIG_M);
                            }
                        }
                        for (int r = 0; r < data.get_nb_routes(); r++)
                        {
                            if (data.start_at_vertex(r, k) && data.is_valid_route(t, i, r))
                            {
                                c_7->SetCoefficient(lambda_[t][i][r], BIG_M);
                            }
                        }
                    }
                }
            }
        }
    }

    // constraints to establish arrival times, considering the traveling times (9) and (10)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    for (auto arc : data.get_route_arcs(r))
                    {
                        int v = arc.out;
                        int k = arc.inc;
                        int a = arc.idx;

                        // y_bar[t][i][k] >= y_[t][i][v] + data.get_distance(a) - BIG_M * (1 - lambda_[t][i][r])
                        // 0 >= y_[t][i][v] + data.get_distance(a) - BIG_M * (1 - lambda_[t][i][r]) - y_bar[t][i][k] >= -inf
                        // -data.get_distance(a) >= y_[t][i][v] - BIG_M + BIG_M * lambda_[t][i][r] - y_bar[t][i][k] >= -inf
                        // -data.get_distance(a) + BIG_M >= y_[t][i][v] + BIG_M * lambda_[t][i][r] - y_bar[t][i][k] >= -inf

                        string name = "traveling_time1(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        MPConstraint* c_8 = solver->MakeRowConstraint(-solver->infinity(), -data.get_distance(a) + BIG_M, name.c_str());
                        c_8->SetCoefficient(y_[t][i][v], 1);
                        c_8->SetCoefficient(lambda_[t][i][r], BIG_M);
                        c_8->SetCoefficient(y_bar_[t][i][k], -1);

                        // y_bar[t][i][k] <= y_[t][i][v] + data.get_distance(a) + BIG_M * (1 - lambda_[t][i][r])
                        // 0 <= y_[t][i][v] + data.get_distance(a) + BIG_M * (1 - lambda_[t][i][r]) - y_bar[t][i][k] <= inf
                        // -data.get_distance(a) <= y_[t][i][v] + BIG_M - BIG_M * lambda_[t][i][r] - y_bar[t][i][k] <= inf
                        // -data.get_distance(a) - BIG_M <= y_[t][i][v] - BIG_M * lambda_[t][i][r] - y_bar[t][i][k] <= inf

                        name = "traveling_time2(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        c_8 = solver->MakeRowConstraint(-data.get_distance(a) - BIG_M, solver->infinity(), name.c_str());
                        c_8->SetCoefficient(y_[t][i][v], 1);
                        c_8->SetCoefficient(lambda_[t][i][r], -BIG_M);
                        c_8->SetCoefficient(y_bar_[t][i][k], -1);
                    }
                }
            }
        }
    }

    // constraints to establhish minimum and maximum service times (11) and (12)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    for (int arc_pos = 0; arc_pos < data.get_route_arcs(r).size() - 1; arc_pos++) // not considering the last arc of the route
                    {
                        Arc arc = data.get_route_arcs(r)[arc_pos];

                        int v = arc.out;
                        int k = arc.inc;
                        int a = arc.idx;

                        // y_[t][i][k] >= y_[t][i][v] + data.get_distance_and_service_min(a) - BIG_M * (1 - lambda_[t][i][r])
                        // 0 >= y_[t][i][v] + data.get_distance_and_service_min(a) - BIG_M * (1 - lambda_[t][i][r]) - y_[t][i][k] >= -inf
                        // -data.get_distance_and_service_min(a) >= y_[t][i][v] - BIG_M + BIG_M * lambda_[t][i][r] - y_[t][i][k] >= -inf
                        // -data.get_distance_and_service_min(a) + BIG_M >= y_[t][i][v] + BIG_M * lambda_[t][i][r] - y_[t][i][k] >= -inf

                        // minimum service time
                        string name = "service_time_min(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        MPConstraint* c_9 = solver->MakeRowConstraint(-solver->infinity(), -data.get_distance_and_service_min(a) + BIG_M, name.c_str());
                        c_9->SetCoefficient(y_[t][i][v], 1);
                        c_9->SetCoefficient(lambda_[t][i][r], BIG_M);
                        c_9->SetCoefficient(y_[t][i][k], -1);

                        // y_[t][i][k] <= y_[t][i][v] + data.get_distance_and_service_max(a) + BIG_M * (1 - lambda_[t][i][r])
                        // 0 <= y_[t][i][v] + data.get_distance_and_service_max(a) + BIG_M * (1 - lambda_[t][i][r]) - y_[t][i][k] <= inf
                        // -data.get_distance_and_service_max(a) <= y_[t][i][v] + BIG_M - BIG_M * lambda_[t][i][r] - y_[t][i][k] <= inf
                        // -data.get_distance_and_service_max(a) - BIG_M <= y_[t][i][v] - BIG_M * lambda_[t][i][r] - y_[t][i][k] <= inf

                        // maximum service time
                        name = "service_time_max(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        c_9 = solver->MakeRowConstraint(-data.get_distance_and_service_max(a) - BIG_M, solver->infinity(), name.c_str());
                        c_9->SetCoefficient(y_[t][i][v], 1);
                        c_9->SetCoefficient(lambda_[t][i][r], -BIG_M);
                        c_9->SetCoefficient(y_[t][i][k], -1);
                    }
                }
            }
        }
    }
}